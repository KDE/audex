/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mmcsectorreader.h"

#include "mmc.h"

#include "core/subchannel.h"

#include <QtMath>

#include <algorithm>
#include <bit>
#include <cstring>

using namespace Qt::StringLiterals;

namespace Audex::Mmc
{

using Cdda::C2Bytes;
using Cdda::SectorBytes;

namespace
{
constexpr int CommandTimeoutMs = 30000;
// A drive reads a few sectors in well under a second. Behind usb-storage
// every timeout also costs a reset of the USB device.
constexpr int ProbeTimeoutMs = 8000;
}

MmcSectorReader::MmcSectorReader(Scsi::Device &device, bool c2Capable, QString description, int maxTransferBytes)
    : m_device(device)
    , m_c2Capable(c2Capable)
    , m_description(std::move(description))
    , m_maxTransferBytes(std::max(maxTransferBytes, SectorBytes + C2Bytes))
{
}

int MmcSectorReader::bytesPerSector() const
{
    return SectorBytes + (m_c2Enabled ? C2Bytes : 0);
}

int MmcSectorReader::maxSectorsPerRead() const
{
    return std::max(1, m_maxTransferBytes / bytesPerSector());
}

bool MmcSectorReader::supportsC2() const
{
    return m_c2Capable;
}

void MmcSectorReader::setC2Enabled(bool enabled)
{
    m_c2Enabled = enabled && m_c2Capable;
}

void MmcSectorReader::setProbeMode(bool enabled)
{
    m_probeMode = enabled;
}

int MmcSectorReader::timeoutMs() const
{
    return m_probeMode ? ProbeTimeoutMs : CommandTimeoutMs;
}

bool MmcSectorReader::setReadSpeed(int factor)
{
    // 1x = 176.4 kB/s (kB = 1000 bytes)
    const int kbps = factor <= 0 ? 0 : int(qCeil(factor * 176.4));
    const bool cdSpeed = note(Mmc::setCdSpeed(m_device, kbps > 0 ? kbps : 0xFFFF)).ok();
    const bool streaming = note(Mmc::setStreaming(m_device, kbps)).ok();
    return cdSpeed || streaming;
}

QByteArray MmcSectorReader::readSubchannelQ(int lba, int count)
{
    constexpr int QBytes = 16; // formatted Q: 10 bytes data, 2 bytes CRC, 4 bytes padding
    const int per = SectorBytes + QBytes;
    QByteArray result;
    for (int done = 0; done < count;) {
        const int n = std::min(count - done, std::max(1, m_maxTransferBytes / per));
        QByteArray raw(qsizetype(n) * per, '\0');
        const Scsi::Result r = note(Mmc::readCd(m_device, lba + done, n, false, raw.data(), raw.size(), true, timeoutMs()));
        const int complete = r.dataUsable() ? std::clamp(r.transferred / per, 0, n) : 0;
        for (int i = 0; i < complete; ++i)
            result.append(raw.constData() + qsizetype(i) * per + SectorBytes, Cdda::SubQBytes);
        if (complete < n)
            return result; // what was read so far
        done += n;
    }
    return result;
}

QByteArray MmcSectorReader::readSubchannelRaw(int lba, int count)
{
    constexpr int RawBytes = 96;
    const int per = SectorBytes + RawBytes;
    QByteArray result;
    for (int done = 0; done < count;) {
        const int n = std::min(count - done, std::max(1, m_maxTransferBytes / per));
        QByteArray raw(qsizetype(n) * per, '\0');
        const Scsi::Result r = note(Mmc::readCdRawSubchannel(m_device, lba + done, n, raw.data(), raw.size(), timeoutMs()));
        const int complete = r.dataUsable() ? std::clamp(r.transferred / per, 0, n) : 0;
        for (int i = 0; i < complete; ++i)
            result.append(raw.constData() + qsizetype(i) * per + SectorBytes, RawBytes);
        if (complete < n)
            return result; // what was read so far
        done += n;
    }
    return result;
}

QString MmcSectorReader::description() const
{
    return m_description;
}

int MmcSectorReader::commandTimeouts() const
{
    return m_timeouts;
}

const Scsi::Result &MmcSectorReader::note(const Scsi::Result &result)
{
    if (result.timedOut())
        ++m_timeouts;
    return result;
}

Rip::SectorReadResult MmcSectorReader::readCommand(int lba, int count, Scsi::Result *scsi)
{
    const int per = bytesPerSector();
    QByteArray raw(qsizetype(count) * per, '\0');
    const Scsi::Result r = note(Mmc::readCd(m_device, lba, count, m_c2Enabled, raw.data(), raw.size(), false, timeoutMs()));
    if (scsi)
        *scsi = r;

    Rip::SectorReadResult result;
    result.audio = QByteArray(qsizetype(count) * SectorBytes, '\0');
    result.sectors.resize(count);
    result.elapsedUs = r.elapsedUs;

    if (!r.dataUsable()) {
        result.error = r.toString();
        return result;
    }

    // A driver that does not report resid leaves it at 0, so transferred is
    // the full length then. 0 bytes means nothing arrived, whatever the status.
    const int completeSectors = std::clamp(r.transferred / per, 0, count);
    // RECOVERED ERROR names the block concerned; without it all are flagged.
    const bool recoveredKnown = r.recovered() && r.sense.informationValid && r.sense.information >= lba && r.sense.information < lba + count;

    for (int i = 0; i < completeSectors; ++i) {
        const char *src = raw.constData() + qsizetype(i) * per;
        std::memcpy(result.audio.data() + qsizetype(i) * SectorBytes, src, SectorBytes);
        Rip::SectorStatus &st = result.sectors[i];
        st.ok = true;
        st.recovered = recoveredKnown ? r.sense.information == lba + i : r.recovered();
        if (m_c2Enabled) {
            int bits = 0;
            for (int b = 0; b < C2Bytes; ++b)
                bits += std::popcount(static_cast<unsigned char>(src[SectorBytes + b]));
            st.c2Errors = quint16(bits);
        }
    }
    if (completeSectors < count)
        result.error = u"short read: %1 of %2 sectors"_s.arg(completeSectors).arg(count);
    return result;
}

Rip::SectorReadResult MmcSectorReader::read(int lba, int count)
{
    Rip::SectorReadResult result = readCommand(lba, count, nullptr);
    if (result.allOk() || count == 1 || m_probeMode)
        return result;

    // One bad sector fails the whole command: retry the burst sector by sector
    // so that the good sectors are not lost.
    result.splitIntoSingleReads = true;
    for (int i = 0; i < count; ++i) {
        if (result.sectors.at(i).ok)
            continue;
        const Rip::SectorReadResult one = readCommand(lba + i, 1, nullptr);
        result.elapsedUs += one.elapsedUs;
        if (one.sectors.value(0).ok) {
            std::memcpy(result.audio.data() + qsizetype(i) * SectorBytes, one.audio.constData(), SectorBytes);
            result.sectors[i] = one.sectors.at(0);
        } else if (!one.error.isEmpty()) {
            result.error = u"LBA %1: %2"_s.arg(lba + i).arg(one.error);
        }
    }
    return result;
}

}
