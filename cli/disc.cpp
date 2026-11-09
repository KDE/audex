/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "commands.h"
#include "common.h"

#include "core/cdg.h"
#include "core/hdcd.h"
#include "metadata/fields.h"
#include "online/accuraterip.h"
#include "online/blockingfetch.h"

#include <QCommandLineParser>
#include <QCoreApplication>

using namespace Qt::StringLiterals;

namespace Audex::Cli
{

namespace
{

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::Cli)
};

QString savedOffsetText(const QString &model)
{
    if (model.isEmpty())
        return Tr::tr("unknown drive model");
    const std::optional<OffsetStore::Entry> saved = OffsetStore::load().find(model);
    if (!saved)
        return Tr::tr("none (measure it with \"audex-cli offset\")");
    return Tr::tr("%1 samples, measured with %n disc(s)", nullptr, int(saved->discs.size())).arg(signedNumber(saved->offset));
}

QString onOff(bool on)
{
    return on ? Tr::tr("on", "HDCD flag") : Tr::tr("off", "HDCD flag");
}
}

int info(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        Tr::tr("Shows the drive and the disc in it: tracks, lengths, data tracks, hidden track, CD-Text, HDCD and CD+G graphics."));
    parser.addHelpOption();
    parser.addPositionalArgument(u"info"_s, Tr::tr("The command."));
    parser.addOption(deviceOption());
    const QCommandLineOption tocOnly(u"toc-only"_s, Tr::tr("Skip the HDCD and CD+G checks (they read a few seconds of the disc)."));
    parser.addOption(tocOnly);
    parser.process(arguments);

    installCancelHandler();

    DriveEntry drive;
    DriveLock lock;
    DiscReadResult disc;
    if (const int code = openDisc(parser.value(deviceOption()), &drive, &lock, &disc); code != ExitOk)
        return code;

    const Cdda::Toc toc = disc.info.toc();
    const QList<int> audio = toc.audioTrackNumbers();
    QStringList out;
    out << field(Tr::tr("Drive"), u"%1 (%2)"_s.arg(disc.driveName, drive.id));
    out << field(Tr::tr("Read offset"), savedOffsetText(disc.driveModel));
    out << field(Tr::tr("Medium"), disc.info.mediumDescription().toString());
    out << field(Tr::tr("Audio"), Tr::tr("%n track(s), %1", nullptr, int(audio.size())).arg(duration(toc.audioEndLba() - toc.audioStartLba())));
    out << field(Tr::tr("Disc IDs"), u"CDDB %1, MusicBrainz %2"_s.arg(disc.info.cddbDiscId(), disc.info.musicBrainzDiscId()));
    if (!disc.discCandidates.isEmpty()) {
        const Metadata::Album &album = disc.discCandidates.first().album;
        out << field(u"CD-Text"_s, u"%1 / %2"_s.arg(album.text(Metadata::Field::Artist), album.text(Metadata::Field::Album)));
    } else {
        out << field(u"CD-Text"_s, Tr::tr("none", "CD-Text"));
    }

    // HDCD: about 10 s from up to 3 tracks; CD+G: a few seconds of sub-channel
    if (!parser.isSet(tocOnly) && !audio.isEmpty()) {
        print(stderr, Tr::tr("Checking for HDCD and CD+G graphics..."));
        OpenedReader opened = openReader(drive);
        if (!opened.reader) {
            out << field(u"HDCD"_s, Tr::tr("not checked (%1)").arg(opened.error)) << field(u"CD+G"_s, Tr::tr("not checked"));
        } else {
            const Hdcd::Result hdcd = Hdcd::probe(*opened.reader, toc, canceled);
            const bool cdg = !canceled() && Cdda::detectCdg(*opened.reader, toc, canceled);
            opened.release();
            if (canceled())
                return ExitCanceled;
            out << field(u"HDCD"_s,
                         hdcd.detected ? Tr::tr("yes (peak extend %1, transient filter %2)").arg(onOff(hdcd.peakExtend), onOff(hdcd.transientFilter))
                                       : Tr::tr("no"));
            out << field(u"CD+G"_s, cdg ? Tr::tr("yes (rip with --cdg for .cdg files)") : Tr::tr("no"));
        }
    }
    out << QString() << u"%1  %2  %3"_s.arg(Tr::tr("Track"), 5).arg(Tr::tr("Start"), 9).arg(Tr::tr("Length"), 9);
    if (toc.htoaSectorCount() > 0)
        out << u"%1  %2  %3  %4"_s.arg(0, 5)
                   .arg(duration(0), 9)
                   .arg(duration(toc.htoaSectorCount()), 9)
                   .arg(disc.htoaSilent ? Tr::tr("hidden track one audio (digital silence)") : Tr::tr("hidden track one audio"));
    for (const Cdda::Track &t : toc.tracks) {
        QStringList flags;
        if (!t.audio)
            flags << Tr::tr("data");
        if (t.preEmphasis)
            flags << Tr::tr("pre-emphasis");
        QString line = u"%1  %2  %3  %4"_s.arg(t.number, 5).arg(duration(t.firstLba), 9).arg(duration(t.sectorCount()), 9).arg(flags.join(u", "_s));
        while (line.endsWith(u' '))
            line.chop(1);
        out << line;
    }
    print(stdout, out.join(u'\n'));
    return ExitOk;
}

int offset(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(Tr::tr("Measures the read offset of the drive with the disc in it, which must be in the AccurateRip database, "
                                            "and saves it for the drive model in %1. Confirm it with one or two other discs.")
                                         .arg(OffsetStore::path()));
    parser.addHelpOption();
    parser.addPositionalArgument(u"offset"_s, Tr::tr("The command."));
    parser.addOption(deviceOption());
    parser.process(arguments);

    installCancelHandler();
    DriveEntry drive;
    DriveLock lock;
    DiscReadResult disc;
    if (const int code = openDisc(parser.value(deviceOption()), &drive, &lock, &disc); code != ExitOk)
        return code;
    const Cdda::Toc toc = disc.info.toc();
    if (toc.audioTrackNumbers().isEmpty()) {
        print(stderr, Tr::tr("The disc has no audio tracks."));
        return ExitError;
    }

    print(stderr, Tr::tr("Looking up the disc in the AccurateRip database..."));
    Text error;
    bool notFound = false;
    const QList<AccurateRip::Response> responses = AccurateRip::lookupDiscEntry(toc, &error, &notFound, 30000, canceled);
    if (canceled())
        return ExitCanceled;
    if (responses.isEmpty()) {
        if (!error.isEmpty() && !notFound) {
            print(stderr, Tr::tr("AccurateRip lookup failed: %1").arg(error.toString()));
            return ExitError;
        }
        print(stderr, Tr::tr("This disc is not in the AccurateRip database. Try a more common one."));
        return ExitUnconfirmed;
    }

    print(stderr, Tr::tr("Measuring the read offset..."));
    OpenedReader opened = openReader(drive);
    if (!opened.reader) {
        print(stderr, u"%1: %2"_s.arg(drive.id, opened.error));
        return ExitError;
    }
    const AccurateRip::OffsetDetection detection = AccurateRip::detectReadOffset(*opened.reader, toc, responses, canceled);
    opened.release();
    if (canceled())
        return ExitCanceled;
    if (!detection.found) {
        print(stderr, Tr::tr("The read offset could not be measured with this disc. %1").arg(detection.details));
        return ExitUnconfirmed;
    }

    QStringList out;
    QStringList warnings;
    out << field(Tr::tr("Drive"), u"%1 (%2)"_s.arg(disc.driveName, drive.id));
    out << field(Tr::tr("Read offset"), Tr::tr("%1 samples").arg(signedNumber(detection.offset)));
    out << field(QString(), detection.details);

    // what other users of the model submitted, as a second opinion
    Text listError;
    const QByteArray list = Online::fetchBlocking(AccurateRip::driveOffsetsUrl(), &listError, nullptr, 30000, canceled);
    const std::optional<AccurateRip::DriveOffset> listed =
        list.isEmpty() ? std::nullopt : AccurateRip::findDriveOffset(AccurateRip::parseDriveOffsets(list), disc.driveModel);
    if (list.isEmpty()) {
        out << field(Tr::tr("Drive list"), listError.isEmpty() ? Tr::tr("not reachable") : Tr::tr("not reachable (%1)").arg(listError.toString()));
    } else if (!listed) {
        out << field(Tr::tr("Drive list"), Tr::tr("this model is not listed at AccurateRip"));
    } else {
        const bool agrees = listed->offset == detection.offset;
        out << field(
            Tr::tr("Drive list"),
            (agrees ? Tr::tr("%1 samples for \"%2\" (agrees)") : Tr::tr("%1 samples for \"%2\" (differs)")).arg(signedNumber(listed->offset), listed->name));
        if (!agrees)
            warnings << Tr::tr("AccurateRip lists %1 samples for this model. Measure again with another disc.").arg(signedNumber(listed->offset));
    }

    if (disc.driveModel.isEmpty()) {
        warnings << Tr::tr("The drive did not tell its model; the offset is not saved. Pass it to \"audex-cli rip\" with --offset.");
    } else {
        OffsetStore store = OffsetStore::load();
        const std::optional<OffsetStore::Entry> before = store.find(disc.driveModel);
        OffsetStore::Entry entry;
        entry.offset = detection.offset;
        const QString discId = disc.info.musicBrainzDiscId();
        if (before && before->offset == detection.offset)
            entry.discs = before->discs;
        if (!entry.discs.contains(discId))
            entry.discs << discId;
        store.set(disc.driveModel, entry);
        QString saveError;
        if (!store.save(&saveError)) {
            print(stderr, Tr::tr("Cannot save %1: %2").arg(OffsetStore::path(), saveError));
            return ExitError;
        }
        out << field(
            Tr::tr("Saved"),
            Tr::tr("%1 samples for \"%2\", measured with %n disc(s)", nullptr, int(entry.discs.size())).arg(signedNumber(entry.offset), disc.driveModel));
        out << field(QString(), Tr::tr("in %1").arg(OffsetStore::path()));
        if (before && before->offset != detection.offset)
            warnings << Tr::tr(
                            "Before, %1 samples were saved (measured with %n disc(s)). A disc whose database entries come from another "
                            "pressing gives a different result; measure again with another disc.",
                            nullptr,
                            int(before->discs.size()))
                            .arg(signedNumber(before->offset));
        else if (entry.discs.size() < 2)
            warnings << Tr::tr("Measured with one disc so far. Confirm it with one or two other discs.");
    }

    print(stdout, out.join(u'\n'));
    for (const QString &warning : std::as_const(warnings))
        print(stderr, Tr::tr("Note: %1").arg(warning));
    return ExitOk;
}

}
