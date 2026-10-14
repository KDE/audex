/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivelock.h"

#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QMutex>

#include <utility>

#ifdef Q_OS_UNIX
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace Audex
{

namespace
{

struct Held {
    int fd = -1;
    int count = 0;
};

// never deleted: a drive thread may still release a lock while Audex quits
QMutex &mutex()
{
    static auto *m = new QMutex;
    return *m;
}

QHash<QString, Held> &held()
{
    static auto *h = new QHash<QString, Held>;
    return *h;
}

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::DriveLock", text);
}

}

DriveLock::~DriveLock()
{
    release();
}

DriveLock::DriveLock(DriveLock &&other) noexcept
    : m_device(std::exchange(other.m_device, QString()))
{
}

DriveLock &DriveLock::operator=(DriveLock &&other) noexcept
{
    if (this != &other) {
        release();
        m_device = std::exchange(other.m_device, QString());
    }
    return *this;
}

DriveLock DriveLock::acquire(const QString &device, QString *error)
{
    DriveLock lock;
    const QMutexLocker locker(&mutex());
    auto it = held().find(device);
    if (it != held().end()) {
        ++it->count;
        lock.m_device = device;
        return lock;
    }

    Held h;
#ifdef Q_OS_UNIX
    // the lock needs no access rights beyond opening the node
    h.fd = ::open(QFile::encodeName(device).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (h.fd < 0) {
        if (error)
            *error = tr("Cannot open %1: %2").arg(device, QString::fromLocal8Bit(std::strerror(errno)));
        return lock;
    }
    if (::flock(h.fd, LOCK_EX | LOCK_NB) != 0) {
        const int e = errno;
        ::close(h.fd);
        if (error)
            *error = e == EWOULDBLOCK ? tr("%1 is in use by another Audex program (Audex or audex-cli).").arg(device)
                                      : tr("Cannot lock %1: %2").arg(device, QString::fromLocal8Bit(std::strerror(e)));
        return lock;
    }
#endif
    h.count = 1;
    held().insert(device, h);
    lock.m_device = device;
    return lock;
}

void DriveLock::release()
{
    if (m_device.isEmpty())
        return;
    const QMutexLocker locker(&mutex());
    auto it = held().find(m_device);
    if (it != held().end() && --it->count <= 0) {
#ifdef Q_OS_UNIX
        if (it->fd >= 0)
            ::close(it->fd); // drops the flock
#endif
        held().erase(it);
    }
    m_device.clear();
}

}
