/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "encoding/encodedoutputs.h"
#include "encoding/registry.h"

#include <QTemporaryDir>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace Audex::Encoding;
using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace
{

struct Gate {
    std::mutex mutex;
    std::condition_variable wake;
    bool entered = false;
    bool released = false;

    bool wait()
    {
        std::unique_lock lock(mutex);
        return wake.wait_for(lock, 2s, [this] {
            return entered;
        });
    }

    void release()
    {
        {
            std::lock_guard lock(mutex);
            released = true;
        }
        wake.notify_all();
    }
};

class GatedFactory : public WavEncoderFactory
{
public:
    explicit GatedFactory(Gate &gate)
        : m_gate(gate)
    {
    }

    std::unique_ptr<AudioEncoder> create(const QVariantMap &settings) const override
    {
        std::unique_lock lock(m_gate.mutex);
        m_gate.entered = true;
        m_gate.wake.notify_all();
        m_gate.wake.wait(lock, [this] {
            return m_gate.released;
        });
        return WavEncoderFactory::create(settings);
    }

private:
    Gate &m_gate;
};

class BlockingEncoder : public AudioEncoder
{
public:
    explicit BlockingEncoder(Gate &gate)
        : m_gate(gate)
    {
    }

    bool open(const QString &, qint64, QString *) override
    {
        return true;
    }

    bool write(QByteArrayView, QString *) override
    {
        std::unique_lock lock(m_gate.mutex);
        m_gate.entered = true;
        m_gate.wake.notify_all();
        m_gate.wake.wait(lock, [this] {
            return m_gate.released;
        });
        return true;
    }

    bool finish(QString *) override
    {
        return true;
    }

    void abort() override
    {
    }

    void interrupt() override
    {
        m_gate.release();
    }

private:
    Gate &m_gate;
};

class BlockingFactory : public WavEncoderFactory
{
public:
    explicit BlockingFactory(Gate &gate)
        : m_gate(gate)
    {
    }

    std::unique_ptr<AudioEncoder> create(const QVariantMap &) const override
    {
        return std::make_unique<BlockingEncoder>(m_gate);
    }

private:
    Gate &m_gate;
};

OutputTarget target(const QString &path)
{
    OutputTarget output;
    output.path = path;
    output.frames = 8;
    return output;
}

}

AUDEX_TEST("cancel prevents creating encoder workers")
{
    QTemporaryDir dir;
    WavEncoderFactory factory;
    EncodedOutputs files(factory, {}, {target(dir.filePath(u"track.wav"_s))}, false);
    files.cancel();
    AUDEX_CHECK(t, !files.write(0, QByteArray(16, 'a')));
    AUDEX_CHECK(t, !files.error().isEmpty());
    AUDEX_CHECK(t, files.writtenFiles().isEmpty());
    AUDEX_CHECK(t, files.finishAll());
}

AUDEX_TEST("cancel during worker creation prevents starting it")
{
    QTemporaryDir dir;
    Gate gate;
    GatedFactory factory(gate);
    const QString path = dir.filePath(u"track.wav"_s);
    EncodedOutputs files(factory, {}, {target(path)}, false);
    bool written = true;
    std::thread producer([&] {
        written = files.write(0, QByteArray(16, 'a'));
    });

    AUDEX_CHECK(t, gate.wait());
    std::thread releaser([&] {
        std::this_thread::sleep_for(100ms);
        gate.release();
    });
    files.cancel();
    {
        std::lock_guard lock(gate.mutex);
        AUDEX_CHECK(t, !gate.released);
    }
    releaser.join();
    producer.join();
    AUDEX_CHECK(t, !written);
    AUDEX_CHECK(t, !QFile::exists(path));
    AUDEX_CHECK(t, !QFile::exists(EncodedOutputs::partialPath(path)));
}

AUDEX_TEST("cancel unblocks queued audio and encoder")
{
    QTemporaryDir dir;
    Gate gate;
    BlockingFactory factory(gate);
    const QString path = dir.filePath(u"track.wav"_s);
    EncodedOutputs files(factory, {}, {target(path)}, false, 16);
    const QByteArray pcm(16, 'a');
    AUDEX_CHECK(t, files.write(0, pcm));
    AUDEX_CHECK(t, gate.wait());

    bool written = true;
    std::thread producer([&] {
        written = files.write(0, pcm);
    });
    files.cancel();
    producer.join();
    files.abortAll();
    AUDEX_CHECK(t, !written);
    AUDEX_CHECK(t, !QFile::exists(path));
}

AUDEX_TEST("discard and cancel can overlap")
{
    QTemporaryDir dir;
    for (int i = 0; i < 32; ++i) {
        Gate gate;
        BlockingFactory factory(gate);
        EncodedOutputs files(factory, {}, {target(dir.filePath(u"track.wav"_s))}, false);
        AUDEX_CHECK(t, files.write(0, QByteArray(16, 'a')));
        AUDEX_CHECK(t, gate.wait());

        std::thread canceler([&] {
            files.cancel();
        });
        AUDEX_CHECK(t, files.discard(0));
        canceler.join();
        AUDEX_CHECK(t, !files.write(0, QByteArray(16, 'a')));
    }
}
