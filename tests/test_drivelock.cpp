/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "device/drivelock.h"

#include <QFile>
#include <QTemporaryDir>

#ifdef Q_OS_UNIX
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

using namespace Audex;
using namespace Qt::StringLiterals;

#ifdef Q_OS_UNIX
AUDEX_TEST("drive lock: shared in the process, refused to another holder")
{
    QTemporaryDir dir;
    const QString node = dir.filePath(u"sr0"_s); // flock works on any file
    QFile f(node);
    AUDEX_CHECK(t, f.open(QIODevice::WriteOnly));
    f.close();

    {
        DriveLock first = DriveLock::acquire(node);
        DriveLock second = DriveLock::acquire(node); // another thread of the same program
        AUDEX_CHECK(t, first.isLocked());
        AUDEX_CHECK(t, second.isLocked());

        // another process: an open file description of its own
        const int other = ::open(QFile::encodeName(node).constData(), O_RDONLY);
        AUDEX_CHECK(t, other >= 0);
        AUDEX_CHECK(t, ::flock(other, LOCK_EX | LOCK_NB) != 0);

        first.release();
        AUDEX_CHECK(t, ::flock(other, LOCK_EX | LOCK_NB) != 0); // second still holds it
        second.release();
        AUDEX_CHECK(t, ::flock(other, LOCK_EX | LOCK_NB) == 0);

        QString error;
        const DriveLock refused = DriveLock::acquire(node, &error);
        AUDEX_CHECK(t, !refused.isLocked());
        AUDEX_CHECK(t, error.contains(u"in use"_s));
        ::close(other);
    }

    DriveLock again = DriveLock::acquire(node);
    AUDEX_CHECK(t, again.isLocked());
    DriveLock moved = std::move(again);
    AUDEX_CHECK(t, moved.isLocked() && !again.isLocked());
}
#endif
