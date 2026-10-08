/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "discsource.h"

#include "core/cdrmanufacturer.h"
#include "device/mmc.h"
#include "device/mmcsectorreader.h"
#include "metadata/cdtextmetadata.h"
#include "sim/simulatedsectorreader.h"

#include <QCoreApplication>
#include <QHash>
#include <QMutex>
#include <QSet>
#include <QThreadPool>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex
{

namespace
{

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::DiscSource", text);
}

// never deleted: a drive thread may still use them while Audex quits; the
// mutex guards the other drive registries as well
QMutex &stalledMutex()
{
    static auto *mutex = new QMutex;
    return *mutex;
}

QSet<QString> &stalledDrives()
{
    static auto *drives = new QSet<QString>;
    return *drives;
}

QHash<QString, bool> &reportedStreams()
{
    static auto *streams = new QHash<QString, bool>;
    return *streams;
}

QString stalledMessage()
{
    return tr("The drive stopped answering. Switch it off and on again (a USB drive: unplug it and plug it in again).");
}

class OwningMmcReader : public Mmc::MmcSectorReader
{
public:
    OwningMmcReader(std::unique_ptr<Scsi::Device> device, bool c2Capable, const QString &name)
        : Mmc::MmcSectorReader(*device, c2Capable, name)
        , m_device(std::move(device))
    {
    }
    Scsi::Device &device()
    {
        return *m_device;
    }

private:
    std::unique_ptr<Scsi::Device> m_device;
};

struct Identified {
    QString name;
    std::optional<Mmc::Capabilities> capabilities;
};

Identified identify(Scsi::Device &device, QStringList *notes)
{
    Identified id;
    QString error;
    if (const auto inq = Mmc::readInquiry(device, &error))
        id.name = u"%1 %2 %3"_s.arg(inq->vendor, inq->model, inq->revision).simplified();
    else if (notes)
        notes->append(error);
    id.capabilities = Mmc::readCapabilities(device, &error);
    if (!id.capabilities && notes)
        notes->append(error);
    return id;
}

// ---- demo disc ------------------------------------------------------------------------

const QList<int> DemoTracks{900, 750, 1200, 600};
constexpr int DemoHtoa = 150;
constexpr quint32 DemoSeed = 2007;
constexpr int DemoNoisySector = 1000;

Sim::SimulatedDisc demoDisc()
{
    Sim::SimulatedDisc disc = Sim::SimulatedDisc::generate(DemoTracks, DemoHtoa, DemoSeed);
    disc.index00 = {{3, disc.toc.track(3)->firstLba - 2 * Cdda::SectorsPerSecond}};
    disc.indexes = {{3, {disc.toc.track(3)->firstLba + 600}}};
    disc.isrc = {{1, u"DEA000000001"_s}, {2, u"DEA000000002"_s}};
    disc.mcn = u"4000000000006"_s;
    return disc;
}

Sim::DriveModel demoDrive()
{
    Sim::DriveModel m;
    m.readOffset = Demo::ReadOffset;
    m.cacheSectors = 256;
    m.seed = DemoSeed;
    return m;
}

}

// ---- drives -------------------------------------------------------------------------------

void markDriveStalled(const QString &device)
{
    const QMutexLocker lock(&stalledMutex());
    stalledDrives().insert(device);
}

void clearDriveStalled(const QString &device)
{
    const QMutexLocker lock(&stalledMutex());
    stalledDrives().remove(device);
}

bool isDriveStalled(const QString &device)
{
    const QMutexLocker lock(&stalledMutex());
    return stalledDrives().contains(device);
}

std::optional<bool> reportedAccurateStream(const QString &device)
{
    const QMutexLocker lock(&stalledMutex());
    const auto it = reportedStreams().constFind(device);
    return it == reportedStreams().constEnd() ? std::nullopt : std::optional<bool>(*it);
}

QThreadPool *driveThreadPool()
{
    static QThreadPool *pool = new QThreadPool; // never deleted: nothing waits for it at exit
    return pool;
}

bool isHtoaSilent(Rip::SectorReader &reader, const Cdda::Toc &toc)
{
    constexpr int MaxSectors = 10 * Cdda::SectorsPerSecond;
    constexpr int OffsetMargin = 3; // a read offset may shift track 1 audio into the last sectors
    if (toc.htoaSectorCount() > MaxSectors)
        return false;
    const int sectors = toc.htoaSectorCount() - OffsetMargin;
    for (int lba = 0; lba < sectors;) {
        const int n = std::min(reader.maxSectorsPerRead(), sectors - lba);
        const Rip::SectorReadResult r = reader.read(lba, n);
        if (!r.allOk() || !std::all_of(r.audio.cbegin(), r.audio.cend(), [](char c) {
                return c == 0;
            }))
            return false;
        lba += n;
    }
    return true;
}

DiscReadResult readDisc(const DriveEntry &drive)
{
    DiscReadResult result;

    if (drive.isSimulated()) {
        const Cdda::Toc toc = Demo::toc();
        result.driveName = tr("Simulated drive");
        result.info = CDInfo(toc, drive.id);
        result.discCandidates = candidatesFromCdText(Demo::cdText(), result.info);
        result.htoaSilent = toc.htoaSectorCount() > 0 && isHtoaSilent(*Demo::reader(), toc);
        result.ok = true;
        return result;
    }

    if (isDriveStalled(drive.id)) {
        result.error = stalledMessage();
        return result;
    }
    Scsi::Device device;
    QString error;
    // a drive that times out here is gone: spare it (and us) the next attempts
    const auto fail = [&](const QString &message) {
        if (device.timeouts() > 0) {
            markDriveStalled(drive.id);
            result.error = stalledMessage();
        } else {
            result.error = message;
        }
        return result;
    };
    if (!device.open(drive.id, &error) || !Mmc::waitUntilReady(device, 30000, &error))
        return fail(error);
    const Identified id = identify(device, &result.notes);
    result.driveName = id.name.isEmpty() ? drive.id : id.name;
    result.c2Known = id.capabilities.has_value();
    result.c2Capable = id.capabilities && id.capabilities->c2Pointers;
    if (id.capabilities) { // the rip warns about a drive without it (RipRequestBuilder::streamWarning())
        const QMutexLocker lock(&stalledMutex());
        reportedStreams().insert(drive.id, id.capabilities->accurateStream);
    }
    if (!device.isWritable())
        result.notes << tr("%1 is opened read-only; the read speed cannot be changed.").arg(drive.id);

    const auto toc = Mmc::readToc(device, &error, &result.notes);
    if (!toc)
        return fail(error);
    result.info = CDInfo(*toc, drive.id);

    QByteArray raw;
    const Scsi::Result r = Mmc::readTocPmaAtip(device, 5, false, 0, raw);
    if (r.dataUsable()) {
        QString cdTextError;
        if (const auto cdText = Cdda::parseCdText(raw, &cdTextError)) {
            result.discCandidates = candidatesFromCdText(*cdText, result.info);
            result.notes += cdText->warnings;
        }
    }

    // --- the medium: a CD-R/RW has an ATIP (from its pregroove), a pressed CD none ---
    QByteArray atipData;
    const std::optional<Mmc::Atip> atip =
        Mmc::readTocPmaAtip(device, 4, true, 0, atipData).dataUsable() ? Mmc::parseAtip(atipData) : std::optional<Mmc::Atip>();
    if (atip) {
        QString details = u"lead-in %1:%2:%3"_s.arg(atip->leadInMinute, 2, 10, QLatin1Char('0'))
                              .arg(atip->leadInSecond, 2, 10, QLatin1Char('0'))
                              .arg(atip->leadInFrame, 2, 10, QLatin1Char('0'));
        const QString manufacturer = Mmc::cdrManufacturer(atip->leadInMinute, atip->leadInSecond, atip->leadInFrame);
        if (!manufacturer.isEmpty())
            details += u", "_s + manufacturer;
        result.info.setMedium(atip->rewritable ? CDInfo::Medium::CdRw : CDInfo::Medium::CdR, details);
    } else if (device.timeouts() == 0) {
        QByteArray configuration;
        const std::optional<quint16> profile =
            Mmc::getConfigurationHeader(device, configuration).dataUsable() ? Mmc::parseCurrentProfile(configuration) : std::optional<quint16>();
        if (profile == 0x0009 || profile == 0x000A)
            result.info.setMedium(*profile == 0x000A ? CDInfo::Medium::CdRw : CDInfo::Medium::CdR);
        // a read-only drive may report a CD-R as CD-ROM too; a writer reads every ATIP
        else if (profile == 0x0008 && id.capabilities && id.capabilities->cdrWrite)
            result.info.setMedium(CDInfo::Medium::Pressed);
    }

    if (toc->htoaSectorCount() > 0) {
        Mmc::MmcSectorReader reader(device, false, result.driveName);
        result.htoaSilent = isHtoaSilent(reader, *toc);
    }
    result.ok = true;
    return result;
}

OpenedReader openReader(const DriveEntry &drive)
{
    OpenedReader opened;
    if (drive.isSimulated()) {
        opened.reader = Demo::reader();
        opened.driveName = opened.reader->description();
        opened.release = [] { };
        return opened;
    }

    if (isDriveStalled(drive.id)) {
        opened.error = stalledMessage();
        return opened;
    }
    auto device = std::make_unique<Scsi::Device>();
    QString error;
    if (!device->open(drive.id, &error) || !Mmc::waitUntilReady(*device, 30000, &error)) {
        if (device->timeouts() > 0) {
            markDriveStalled(drive.id);
            error = stalledMessage();
        }
        opened.error = error;
        return opened;
    }
    const Identified id = identify(*device, nullptr);
    const bool c2 = id.capabilities && id.capabilities->c2Pointers;
    const bool locked = Mmc::preventMediumRemoval(*device, true).ok();
    auto reader = std::make_unique<OwningMmcReader>(std::move(device), c2, id.name.isEmpty() ? drive.id : id.name);
    OwningMmcReader *raw = reader.get();
    opened.driveName = reader->description();
    opened.release = [raw, locked, node = drive.id] {
        // Unlocking also shows whether the drive still answers: one that does
        // not would keep every further command waiting for its timeout.
        if (locked && Mmc::preventMediumRemoval(raw->device(), false).timedOut()) {
            markDriveStalled(node);
            return;
        }
        raw->setReadSpeed(0);
    };
    opened.reader = std::move(reader);
    return opened;
}

// ---- demo ------------------------------------------------------------------------------------

namespace Demo
{

Cdda::Toc toc()
{
    return demoDisc().toc;
}

Cdda::CdText cdText()
{
    using namespace Cdda;
    CdTextBlock block;
    block.hasSizeInfo = true;
    block.languageCode = 0x09;
    block.firstTrack = 1;
    block.lastTrack = 4;
    block.texts[CdTextPack::Title] = {
        {0, u"Simulated Sessions"_s},
        {1, u"Offset Blues"_s},
        {2, u"Cache Me If You Can"_s},
        {3, u"The Noisy Sector"_s},
        {4, u"Lead-Out"_s},
    };
    block.texts[CdTextPack::Performer] = {{0, u"The Test Tones"_s},
                                          {1, u"The Test Tones"_s},
                                          {2, u"The Test Tones"_s},
                                          {3, u"The Test Tones feat. C2"_s},
                                          {4, u"The Test Tones"_s}};
    block.texts[CdTextPack::UpcIsrc] = {{0, u"4000000000006"_s}, {1, u"DEA000000001"_s}};
    block.genreCode = 0x0017;
    CdText cdText;
    cdText.blocks.append(block);
    return cdText;
}

std::unique_ptr<Rip::SectorReader> reader()
{
    auto reader = std::make_unique<Sim::SimulatedSectorReader>(demoDisc(), demoDrive());
    reader->addDefect(DemoNoisySector, Sim::Defect{Sim::DefectKind::Noisy, 0.3, 0});
    return reader;
}

QByteArray expectedAudio(int firstLba, int lastLba, int correctionOffset)
{
    const Sim::SimulatedSectorReader reader(demoDisc(), demoDrive());
    const Cdda::Toc t = reader.disc().toc;
    return Sim::expectedAudio(reader, firstLba, lastLba, correctionOffset, false, {t.audioStartLba(), t.audioEndLba()});
}

}

}
