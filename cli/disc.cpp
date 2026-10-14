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

using namespace Qt::StringLiterals;

namespace Audex::Cli
{

namespace
{

QString savedOffsetText(const QString &model)
{
    if (model.isEmpty())
        return u"unknown drive model"_s;
    const std::optional<OffsetStore::Entry> saved = OffsetStore::load().find(model);
    if (!saved)
        return u"none (measure it with \"audex-cli offset\")"_s;
    return u"%1 samples, measured with %2 disc(s)"_s.arg(signedNumber(saved->offset)).arg(saved->discs.size());
}

}

int info(const QStringList &arguments)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Shows the drive and the disc in it: tracks, lengths, data tracks, hidden track, CD-Text, HDCD and CD+G graphics."_s);
    parser.addHelpOption();
    parser.addPositionalArgument(u"info"_s, u"The command."_s);
    parser.addOption(deviceOption());
    const QCommandLineOption tocOnly(u"toc-only"_s, u"Skip the HDCD and CD+G checks (they read a few seconds of the disc)."_s);
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
    out << u"Drive:        %1 (%2)"_s.arg(disc.driveName, drive.id);
    out << u"Read offset:  %1"_s.arg(savedOffsetText(disc.driveModel));
    out << u"Medium:       %1"_s.arg(disc.info.mediumDescription());
    out << u"Audio:        %1 track(s), %2"_s.arg(audio.size()).arg(duration(toc.audioEndLba() - toc.audioStartLba()));
    out << u"Disc IDs:     CDDB %1, MusicBrainz %2"_s.arg(disc.info.cddbDiscId(), disc.info.musicBrainzDiscId());
    if (!disc.discCandidates.isEmpty()) {
        const Metadata::Album &album = disc.discCandidates.first().album;
        out << u"CD-Text:      %1 / %2"_s.arg(album.text(Metadata::Field::Artist), album.text(Metadata::Field::Album));
    } else {
        out << u"CD-Text:      none"_s;
    }

    // HDCD: about 10 s from up to 3 tracks; CD+G: a few seconds of sub-channel
    if (!parser.isSet(tocOnly) && !audio.isEmpty()) {
        print(stderr, u"Checking for HDCD and CD+G graphics..."_s);
        OpenedReader opened = openReader(drive);
        if (!opened.reader) {
            out << u"HDCD:         not checked (%1)"_s.arg(opened.error) << u"CD+G:         not checked"_s;
        } else {
            const Hdcd::Result hdcd = Hdcd::probe(*opened.reader, toc, canceled);
            const bool cdg = !canceled() && Cdda::detectCdg(*opened.reader, toc, canceled);
            opened.release();
            if (canceled())
                return ExitCanceled;
            out << u"HDCD:         %1"_s.arg(hdcd.detected ? u"yes (peak extend %1, transient filter %2)"_s.arg(hdcd.peakExtend ? u"on"_s : u"off"_s,
                                                                                                                hdcd.transientFilter ? u"on"_s : u"off"_s)
                                                           : u"no"_s);
            out << u"CD+G:         %1"_s.arg(cdg ? u"yes (rip with --cdg for .cdg files)"_s : u"no"_s);
        }
    }
    out << QString() << u"Track      Start     Length"_s;
    if (toc.htoaSectorCount() > 0)
        out << u"%1  %2  %3  hidden track one audio%4"_s.arg(0, 5)
                   .arg(duration(0), 9)
                   .arg(duration(toc.htoaSectorCount()), 9)
                   .arg(disc.htoaSilent ? u" (digital silence)"_s : QString());
    for (const Cdda::Track &t : toc.tracks) {
        QStringList flags;
        if (!t.audio)
            flags << u"data"_s;
        if (t.preEmphasis)
            flags << u"pre-emphasis"_s;
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
    parser.setApplicationDescription(
        u"Measures the read offset of the drive with the disc in it, which must be in the AccurateRip database, "
        "and saves it for the drive model in %1. Confirm it with one or two other discs."_s.arg(OffsetStore::path()));
    parser.addHelpOption();
    parser.addPositionalArgument(u"offset"_s, u"The command."_s);
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
        print(stderr, u"The disc has no audio tracks."_s);
        return ExitError;
    }

    print(stderr, u"Looking up the disc in the AccurateRip database..."_s);
    QString error;
    bool notFound = false;
    const QList<AccurateRip::Response> responses = AccurateRip::lookupDiscEntry(toc, &error, &notFound, 30000, canceled);
    if (canceled())
        return ExitCanceled;
    if (responses.isEmpty()) {
        if (!error.isEmpty() && !notFound) {
            print(stderr, u"AccurateRip lookup failed: %1"_s.arg(error));
            return ExitError;
        }
        print(stderr, u"This disc is not in the AccurateRip database. Try a more common one."_s);
        return ExitUnconfirmed;
    }

    print(stderr, u"Measuring the read offset..."_s);
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
        print(stderr, u"The read offset could not be measured with this disc. %1"_s.arg(detection.details));
        return ExitUnconfirmed;
    }

    QStringList out;
    QStringList warnings;
    out << u"Drive:        %1 (%2)"_s.arg(disc.driveName, drive.id);
    out << u"Read offset:  %1 samples"_s.arg(signedNumber(detection.offset));
    out << u"              %1"_s.arg(detection.details);

    // what other users of the model submitted, as a second opinion
    QString listError;
    const QByteArray list = Online::fetchBlocking(AccurateRip::driveOffsetsUrl(), &listError, nullptr, 30000, canceled);
    const std::optional<AccurateRip::DriveOffset> listed =
        list.isEmpty() ? std::nullopt : AccurateRip::findDriveOffset(AccurateRip::parseDriveOffsets(list), disc.driveModel);
    if (list.isEmpty()) {
        out << u"Drive list:   not reachable%1"_s.arg(listError.isEmpty() ? QString() : u" (%1)"_s.arg(listError));
    } else if (!listed) {
        out << u"Drive list:   this model is not listed at AccurateRip"_s;
    } else {
        const bool agrees = listed->offset == detection.offset;
        out << u"Drive list:   %1 samples for \"%2\" (%3)"_s.arg(signedNumber(listed->offset), listed->name, agrees ? u"agrees"_s : u"differs"_s);
        if (!agrees)
            warnings << u"AccurateRip lists %1 samples for this model. Measure again with another disc."_s.arg(signedNumber(listed->offset));
    }

    if (disc.driveModel.isEmpty()) {
        warnings << u"The drive did not tell its model; the offset is not saved. Pass it to \"audex-cli rip\" with --offset."_s;
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
        if (!store.save(&error)) {
            print(stderr, u"Cannot save %1: %2"_s.arg(OffsetStore::path(), error));
            return ExitError;
        }
        out << u"Saved:        %1 samples for \"%2\", measured with %3 disc(s)"_s.arg(signedNumber(entry.offset), disc.driveModel).arg(entry.discs.size());
        out << u"              in %1"_s.arg(OffsetStore::path());
        if (before && before->offset != detection.offset)
            warnings << u"Before, %1 samples were saved (measured with %2 disc(s)). A disc whose database entries come from another "
                        "pressing gives a different result; measure again with another disc."_s.arg(signedNumber(before->offset))
                            .arg(before->discs.size());
        else if (entry.discs.size() < 2)
            warnings << u"Measured with one disc so far. Confirm it with one or two other discs."_s;
    }

    print(stdout, out.join(u'\n'));
    for (const QString &warning : std::as_const(warnings))
        print(stderr, u"Note: "_s + warning);
    return ExitOk;
}

}
