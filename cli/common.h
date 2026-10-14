/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QMap>
#include <QString>
#include <QStringList>

#include <cstdio>
#include <optional>

#include "device/drivelock.h"
#include "utils/discsource.h"

class QCommandLineOption;

namespace Audex::Cli
{

// exit codes, also documented in the README
enum ExitCode {
    ExitOk = 0,
    ExitFailed = 1, // a file changed, a database contradicts the rip, unreadable sectors
    ExitUnconfirmed = 2, // nothing is wrong, but not every track is confirmed
    ExitError = 3, // could not be done (drive, disc, files, databases)
    ExitUsage = 64,
    ExitCanceled = 130 // Ctrl+C
};

void print(FILE *stream, const QString &text);
QString hex8(quint32 v);
QString signedNumber(int v); // "+6", "-12", "0"
QString duration(int sectors); // "4:12.33" (minutes:seconds.frames)

// Ctrl+C sets a flag the long operations look at; a second one ends the
// program at once.
void installCancelHandler();
bool canceled();

// The drive: the device node given, else the only optical drive of the system
std::optional<DriveEntry> chooseDrive(const QString &device, QString *error);

// -d/--device
QCommandLineOption deviceOption();

// Chooses the drive and reads the disc, keeping the drive locked against
// other Audex processes from here on. Returns ExitOk or the exit code; the
// error is printed.
int openDisc(const QString &device, DriveEntry *drive, DriveLock *lock, DiscReadResult *disc);

// Read offsets per drive model, measured by "audex-cli offset", in
// ~/.config/audex-cli.conf. A line per model, readable and editable:
//   HL-DT-ST BD-RE WH16NS60 = +6  # discs: <MusicBrainz disc IDs>
// The discs are those whose measurement gave this offset.
class OffsetStore
{
public:
    struct Entry {
        int offset = 0;
        QStringList discs;
    };

    static QString path();
    static OffsetStore load();

    std::optional<Entry> find(const QString &model) const;
    void set(const QString &model, const Entry &entry);
    bool save(QString *error) const;

private:
    QMap<QString, Entry> m_entries;
};

}
