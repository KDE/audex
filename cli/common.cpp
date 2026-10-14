/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "common.h"

#include "core/cdda.h"

#include <QCommandLineOption>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

#include <atomic>
#include <csignal>

using namespace Qt::StringLiterals;

namespace Audex::Cli
{

namespace
{

std::atomic<bool> s_canceled{false};

void onInterrupt(int)
{
    s_canceled = true;
    std::signal(SIGINT, SIG_DFL); // the second Ctrl+C ends the program
}

}

void print(FILE *stream, const QString &text)
{
    std::fputs(text.toLocal8Bit().constData(), stream);
    std::fputc('\n', stream);
    std::fflush(stream);
}

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0')).toUpper();
}

QString signedNumber(int v)
{
    return v > 0 ? u"+%1"_s.arg(v) : QString::number(v);
}

QString duration(int sectors)
{
    const int frames = sectors % Cdda::SectorsPerSecond;
    const int seconds = sectors / Cdda::SectorsPerSecond;
    return u"%1:%2.%3"_s.arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0')).arg(frames, 2, 10, QLatin1Char('0'));
}

void installCancelHandler()
{
    std::signal(SIGINT, onInterrupt);
}

bool canceled()
{
    return s_canceled;
}

std::optional<DriveEntry> chooseDrive(const QString &device, QString *error)
{
    if (!device.isEmpty())
        return DriveEntry{device, device};

    // Linux: /dev/sr0, FreeBSD: /dev/cd0
    const QStringList nodes = QDir(u"/dev"_s).entryList({u"sr[0-9]*"_s, u"cd[0-9]*"_s}, QDir::System, QDir::Name);
    if (nodes.size() == 1) {
        const QString node = u"/dev/"_s + nodes.first();
        return DriveEntry{node, node};
    }
    if (error) {
        QStringList paths;
        for (const QString &node : nodes)
            paths << u"/dev/"_s + node;
        *error = nodes.isEmpty() ? u"No optical drive found; name one with --device."_s
                                 : u"Several drives found (%1); choose one with --device."_s.arg(paths.join(u", "_s));
    }
    return std::nullopt;
}

QCommandLineOption deviceOption()
{
    return QCommandLineOption({u"d"_s, u"device"_s}, u"The drive, e.g. /dev/sr0 (default: the only one)."_s, u"node"_s);
}

int openDisc(const QString &device, DriveEntry *drive, DriveLock *lock, DiscReadResult *disc)
{
    QString error;
    const std::optional<DriveEntry> chosen = chooseDrive(device, &error);
    if (!chosen) {
        print(stderr, error);
        return ExitError;
    }
    *drive = *chosen;
    if (!drive->isSimulated()) {
        *lock = DriveLock::acquire(drive->id, &error);
        if (!lock->isLocked()) {
            print(stderr, error);
            return ExitError;
        }
    }
    *disc = readDisc(*drive);
    if (!disc->ok) {
        print(stderr, u"%1: %2"_s.arg(drive->id, disc->error));
        return ExitError;
    }
    return ExitOk;
}

QString OffsetStore::path()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/audex-cli.conf"_s;
}

OffsetStore OffsetStore::load()
{
    OffsetStore store;
    QFile file(path());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return store;
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const qsizetype hash = line.indexOf(u'#');
        const QString data = (hash < 0 ? line : line.left(hash)).trimmed();
        const qsizetype equals = data.lastIndexOf(u'=');
        if (data.isEmpty() || equals <= 0)
            continue;
        bool ok = false;
        Entry entry;
        entry.offset = data.mid(equals + 1).trimmed().toInt(&ok);
        if (!ok)
            continue;
        if (hash >= 0) {
            QString comment = line.mid(hash + 1).trimmed();
            if (comment.startsWith(u"discs:"_s))
                for (const QString &disc : comment.mid(6).split(u',', Qt::SkipEmptyParts))
                    entry.discs << disc.trimmed();
        }
        store.m_entries.insert(data.left(equals).simplified(), entry);
    }
    return store;
}

std::optional<OffsetStore::Entry> OffsetStore::find(const QString &model) const
{
    const auto it = m_entries.constFind(model.simplified());
    if (it == m_entries.cend())
        return std::nullopt;
    return *it;
}

void OffsetStore::set(const QString &model, const Entry &entry)
{
    m_entries.insert(model.simplified(), entry);
}

bool OffsetStore::save(QString *error) const
{
    QDir().mkpath(QFileInfo(path()).path());
    QSaveFile file(path());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    QTextStream out(&file);
    out << "# Read offsets of drive models, measured by \"audex-cli offset\" (samples,\n"
           "# AccurateRip convention). \"discs\" are the discs whose measurement gave\n"
           "# this offset (MusicBrainz disc IDs).\n";
    for (auto it = m_entries.cbegin(); it != m_entries.cend(); ++it) {
        out << it.key() << " = " << signedNumber(it->offset);
        if (!it->discs.isEmpty())
            out << "  # discs: " << it->discs.join(u", "_s);
        out << '\n';
    }
    out.flush();
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

}
