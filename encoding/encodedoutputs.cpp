/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "encodedoutputs.h"

#include "utils/encodercommand.h"

#include <deque>
#include <thread>

using namespace Qt::StringLiterals;

namespace Audex::Encoding
{

namespace
{
QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::EncodedOutputs", text);
}

// what a command line encoder may put into its arguments; an image file has
// no track of its own, so it uses the album data
QMap<QString, QString> commandValues(const TagInfo &tags)
{
    using Metadata::Field;
    const bool image = tags.trackNumber < 0;
    QString artist = image ? QString() : tags.track().text(Field::Artist);
    if (artist.isEmpty())
        artist = tags.album.text(Field::Artist);
    const QString title = image ? tags.album.text(Field::Album) : tags.track().text(Field::Title);
    return trackValues(artist, title, image ? 1 : tags.displayTrackNumber, image ? QString() : tags.track().text(Field::ISRC));
}
}

// ---- ByteBudget ----------------------------------------------------------------------------

bool ByteBudget::acquire(qint64 bytes)
{
    std::unique_lock lock(m_mutex);
    // a single chunk larger than the limit is allowed when nothing else is queued
    m_available.wait(lock, [&] {
        return m_canceled || m_used == 0 || m_used + bytes <= m_limit;
    });
    if (m_canceled)
        return false;
    m_used += bytes;
    return true;
}

void ByteBudget::cancel()
{
    {
        std::lock_guard lock(m_mutex);
        m_canceled = true;
    }
    m_available.notify_all();
}

void ByteBudget::release(qint64 bytes)
{
    {
        std::lock_guard lock(m_mutex);
        m_used -= bytes;
    }
    m_available.notify_all();
}

// ---- EncoderWorker -----------------------------------------------------------------------------

class EncoderWorker
{
public:
    EncoderWorker(std::unique_ptr<AudioEncoder> encoder, const QString &path, qint64 frames, const TagInfo &tags, bool writeTags, ByteBudget &budget)
        : m_encoder(std::move(encoder))
        , m_path(path)
        , m_partPath(EncodedOutputs::partialPath(path))
        , m_frames(frames)
        , m_tags(tags)
        , m_writeTags(writeTags)
        , m_budget(budget)
    {
    }

    ~EncoderWorker()
    {
        abort();
    }

    void start()
    {
        m_thread = std::thread([this] {
            run();
        });
    }

    bool push(QByteArrayView pcm)
    {
        if (hasFailed())
            return false;
        const qint64 size = pcm.size();
        if (!m_budget.acquire(size))
            return false;
        QByteArray copy = pcm.toByteArray(); // deep copy outside the lock
        {
            std::lock_guard lock(m_mutex);
            if (m_failed || m_abort || m_closed) { // nobody would drain the queue
                m_budget.release(size);
                return false;
            }
            m_queue.push_back(std::move(copy));
        }
        m_wake.notify_all();
        return true;
    }

    void close()
    {
        {
            std::lock_guard lock(m_mutex);
            m_closed = true;
        }
        m_wake.notify_all();
    }

    void wait()
    {
        std::lock_guard lock(m_joinMutex);
        if (m_thread.joinable())
            m_thread.join();
    }

    void interrupt()
    {
        {
            std::lock_guard lock(m_mutex);
            m_abort = true;
        }
        m_wake.notify_all();
        m_encoder->interrupt();
    }

    void abort()
    {
        interrupt();
        wait();
    }

    bool hasFailed() const
    {
        std::lock_guard lock(m_mutex);
        return m_failed;
    }
    bool isClosed() const
    {
        std::lock_guard lock(m_mutex);
        return m_closed;
    }
    bool succeeded() const
    {
        std::lock_guard lock(m_mutex);
        return m_succeeded;
    }
    QString error() const
    {
        std::lock_guard lock(m_mutex);
        return m_error;
    }
    QStringList warnings() const
    {
        std::lock_guard lock(m_mutex);
        return m_warnings;
    }
    QString path() const
    {
        return m_path;
    }

private:
    void setFailed(const QString &error)
    {
        std::lock_guard lock(m_mutex);
        m_failed = true;
        m_error = QFileInfo(m_path).fileName() + u": "_s + error;
    }

    void dropQueue()
    {
        std::lock_guard lock(m_mutex);
        for (const QByteArray &chunk : m_queue)
            m_budget.release(chunk.size());
        m_queue.clear();
    }

    void discard()
    {
        m_encoder->abort();
        dropQueue();
        QFile::remove(m_partPath);
    }

    void run()
    {
        QString error;
        QDir().mkpath(QFileInfo(m_partPath).absolutePath());
        m_encoder->setTrackValues(commandValues(m_tags));
        if (!m_encoder->open(m_partPath, m_frames, &error)) {
            setFailed(error);
            discard();
            return;
        }

        QByteArray carry; // encoders always get whole frames
        while (true) {
            QByteArray chunk;
            {
                std::unique_lock lock(m_mutex);
                m_wake.wait(lock, [this] {
                    return !m_queue.empty() || m_closed || m_abort;
                });
                if (m_abort)
                    break;
                if (m_queue.empty())
                    break; // closed and drained
                chunk = std::move(m_queue.front());
                m_queue.pop_front();
            }
            const qint64 queued = chunk.size();
            if (!carry.isEmpty()) {
                chunk.prepend(carry);
                carry.clear();
            }
            const qsizetype whole = chunk.size() - chunk.size() % BytesPerFrame;
            if (whole < chunk.size())
                carry = chunk.mid(whole);
            const bool ok = whole == 0 || m_encoder->write(QByteArrayView(chunk).first(whole), &error);
            m_budget.release(queued);
            if (!ok) {
                setFailed(error);
                break;
            }
        }

        bool aborted;
        {
            std::lock_guard lock(m_mutex);
            aborted = m_abort;
        }
        if (aborted || hasFailed()) {
            discard();
            return;
        }

        if (!m_encoder->finish(&error)) {
            setFailed(error);
            discard();
            return;
        }

        if (!QFile::exists(m_partPath)) {
            setFailed(tr("the encoder did not create %1").arg(m_partPath));
            return;
        }

        const QString suffix = QFileInfo(m_path).suffix();
        if (m_writeTags && TagWriter::supportsSuffix(suffix) && !TagWriter::write(m_partPath, suffix, m_tags, &error)) {
            std::lock_guard lock(m_mutex);
            m_warnings << QFileInfo(m_path).fileName() + u": "_s + error;
        }

        QFile::remove(m_path);
        if (!QFile::rename(m_partPath, m_path)) {
            setFailed(tr("cannot rename %1").arg(m_partPath));
            QFile::remove(m_partPath);
            return;
        }
        std::lock_guard lock(m_mutex);
        m_succeeded = true;
    }

    std::unique_ptr<AudioEncoder> m_encoder;
    QString m_path;
    QString m_partPath;
    qint64 m_frames;
    TagInfo m_tags;
    bool m_writeTags;
    ByteBudget &m_budget;

    std::thread m_thread;
    std::mutex m_joinMutex; // wait() may be called from several places
    mutable std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<QByteArray> m_queue;
    bool m_closed = false;
    bool m_abort = false;
    bool m_failed = false;
    bool m_succeeded = false;
    QString m_error;
    QStringList m_warnings;
};

// ---- EncodedOutputs ----------------------------------------------------------------------------

EncodedOutputs::EncodedOutputs(const EncoderFactory &factory,
                               const QVariantMap &settings,
                               const QList<OutputTarget> &targets,
                               bool writeTags,
                               qint64 memoryLimit)
    : m_factory(factory)
    , m_settings(settings)
    , m_targets(targets)
    , m_writeTags(writeTags)
    , m_budget(memoryLimit)
{
}

EncodedOutputs::~EncodedOutputs()
{
    abortAll();
}

QString EncodedOutputs::partialPath(const QString &path)
{
    const QFileInfo info(path);
    if (info.suffix().isEmpty())
        return path + u".part"_s;
    return QDir(info.path()).filePath(info.completeBaseName() + u".part."_s + info.suffix());
}

EncoderWorker *EncodedOutputs::workerFor(int segment)
{
    const QString path = m_targets.value(segment).path;
    if (path.isEmpty())
        return nullptr;
    const auto it = m_workers.find(path);
    if (it != m_workers.end())
        return it->second.get();

    // segments arrive in order: the previous file is complete and can
    // finish in the background while the extraction continues
    for (auto &[p, worker] : m_workers)
        worker->close();

    qint64 frames = 0;
    for (const OutputTarget &t : std::as_const(m_targets))
        if (t.path == path)
            frames += t.frames;

    auto worker = std::make_unique<EncoderWorker>(m_factory.create(m_settings), path, frames, m_targets.value(segment).tags, m_writeTags, m_budget);
    EncoderWorker *raw = worker.get();
    {
        std::lock_guard lock(m_workersMutex);
        m_workers.emplace(path, std::move(worker));
    }
    raw->start();
    return raw;
}

bool EncodedOutputs::write(int segment, QByteArrayView pcm)
{
    EncoderWorker *worker = workerFor(segment);
    if (!worker)
        return true; // dry run
    if (worker->push(pcm))
        return true;
    m_error = worker->error();
    if (m_error.isEmpty() && worker->isClosed())
        m_error = tr("%1 is already complete; segments of one file must be consecutive.").arg(worker->path());
    if (m_error.isEmpty())
        m_error = tr("Encoding was aborted.");
    return false;
}

bool EncodedOutputs::finishAll()
{
    for (auto &[path, worker] : m_workers)
        worker->close();
    bool ok = true;
    // report in the order of the targets
    QStringList order;
    for (const OutputTarget &t : std::as_const(m_targets))
        if (!t.path.isEmpty() && !order.contains(t.path))
            order << t.path;
    for (const QString &path : std::as_const(order)) {
        const auto it = m_workers.find(path);
        if (it == m_workers.end())
            continue;
        EncoderWorker &w = *it->second;
        w.wait();
        m_warnings += w.warnings();
        if (w.succeeded()) {
            if (!m_written.contains(path))
                m_written << path;
        } else {
            ok = false;
            if (m_error.isEmpty())
                m_error = w.error();
        }
    }
    return ok;
}

void EncodedOutputs::abortAll()
{
    for (auto &[path, worker] : m_workers)
        if (!worker->succeeded())
            worker->abort();
}

bool EncodedOutputs::discard(int segment)
{
    const QString path = m_targets.value(segment).path;
    if (path.isEmpty())
        return false;
    for (int i = 0; i < m_targets.size(); ++i)
        if (i != segment && m_targets.at(i).path == path)
            return false;
    const auto it = m_workers.find(path);
    if (it == m_workers.end())
        return true;
    it->second->abort();
    if (it->second->succeeded())
        QFile::remove(path);
    {
        std::lock_guard lock(m_workersMutex);
        m_workers.erase(it);
    }
    m_written.removeAll(path);
    return true;
}

void EncodedOutputs::cancel()
{
    m_budget.cancel();
    std::lock_guard lock(m_workersMutex);
    for (auto &[path, worker] : m_workers)
        worker->interrupt();
}

QString EncodedOutputs::error() const
{
    return m_error;
}

QStringList EncodedOutputs::writtenFiles() const
{
    return m_written;
}

QStringList EncodedOutputs::warnings() const
{
    return m_warnings;
}

}
