/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArrayView>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QVariantMap>

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>

#include "encoder.h"
#include "tagwriter.h"

namespace Audex::Encoding
{

struct OutputTarget {
    QString path; // final file name; segments sharing a path go into one file
    qint64 frames = 0; // stereo frames of this segment
    TagInfo tags; // used once per file (from the first segment)
};

// Shared memory limit for all encoder queues: the extraction waits when the
// encoders fall behind.
//
// The pipeline uses std::mutex/std::condition_variable/std::thread on purpose:
// they go straight through pthread, so ThreadSanitizer can check this code
// even against a Qt that was not built with TSan (QWaitCondition releases the
// mutex inside libQt6Core, which an uninstrumented Qt hides from TSan).
class ByteBudget
{
public:
    explicit ByteBudget(qint64 limit)
        : m_limit(limit)
    {
    }
    bool acquire(qint64 bytes); // false once canceled
    void release(qint64 bytes);
    void cancel();

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_available;
    qint64 m_limit;
    qint64 m_used = 0;
    bool m_canceled = false;
};

class EncoderWorker;

// Maps the segments of a rip to files and encodes them in worker threads.
// Several segments may share one path (image). Every file is written as
// "<name>.part.<suffix>", tagged, and renamed when complete. Aborted or
// failed files are removed.
class EncodedOutputs
{
public:
    EncodedOutputs(const EncoderFactory &factory,
                   const QVariantMap &settings,
                   const QList<OutputTarget> &targets,
                   bool writeTags = true,
                   qint64 memoryLimit = 128 * 1024 * 1024);
    ~EncodedOutputs(); // aborts unfinished files

    bool write(int segment, QByteArrayView pcm); // may block (back pressure)
    bool finishAll();
    void abortAll();
    void cancel(); // thread-safe: unblocks write() and interrupts the encoders

    // Forgets a segment that has its own file (removed, also if already
    // complete); the next write() for it starts the file again. False if the
    // file is shared with other segments (image).
    bool discard(int segment);

    QString error() const;
    QStringList writtenFiles() const;
    QStringList warnings() const; // e.g. tagging problems

    static QString partialPath(const QString &path);

private:
    EncoderWorker *workerFor(int segment);

    const EncoderFactory &m_factory;
    QVariantMap m_settings;
    QList<OutputTarget> m_targets;
    bool m_writeTags;
    ByteBudget m_budget;
    std::map<QString, std::unique_ptr<EncoderWorker>> m_workers;
    std::mutex m_workersMutex; // guards inserting into m_workers against cancel()
    QString m_error;
    QStringList m_written;
    QStringList m_warnings;
};

}
