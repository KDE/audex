/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "mmc.h"

#include <QElapsedTimer>
#include <QThread>
#include <QtEndian>

#include <array>

using namespace Qt::StringLiterals;

namespace Audex::Mmc
{

namespace
{

constexpr quint8 OpTestUnitReady = 0x00;
constexpr quint8 OpInquiry = 0x12;
constexpr quint8 OpPreventAllow = 0x1E;
constexpr quint8 OpReadTocPmaAtip = 0x43;
constexpr quint8 OpGetConfiguration = 0x46;
constexpr quint8 OpModeSense10 = 0x5A;
constexpr quint8 OpSetStreaming = 0xB6;
constexpr quint8 OpSetCdSpeed = 0xBB;
constexpr quint8 OpReadCd = 0xBE;

using Cdb = std::array<char, 12>;

QByteArrayView view(const Cdb &cdb, int length)
{
    return QByteArrayView(cdb.data(), length);
}

void putBe16(Cdb &cdb, int pos, quint16 v)
{
    cdb[pos] = char(v >> 8);
    cdb[pos + 1] = char(v);
}

quint16 be16(const QByteArray &d, int pos)
{
    return qFromBigEndian<quint16>(d.constData() + pos);
}

template<std::size_t N>
void putBe32(std::array<char, N> &buffer, int pos, quint32 v)
{
    qToBigEndian<quint32>(v, buffer.data() + pos);
}

// Two-step read of a response with a 2 byte length header at `lengthOffset`
// (value excludes the bytes up to and including the length field).
Scsi::Result
readWithLengthHeader(Scsi::Device &device, Cdb cdb, int cdbLength, int allocOffset, int headerSize, int lengthOffset, int descriptorSize, QByteArray &response)
{
    response.clear();

    QByteArray header(headerSize, '\0');
    putBe16(cdb, allocOffset, quint16(headerSize));
    Scsi::Result r = device.execute(view(cdb, cdbLength), header.data(), header.size(), Scsi::Direction::FromDevice);
    if (!r.dataUsable())
        return r;

    int total = int(be16(header, lengthOffset)) + lengthOffset + 2;

    // Some firmwares report the size of the returned instead of the available
    // data. If the reported size looks implausible, ask for the maximum.
    const bool implausible = total <= headerSize || (descriptorSize > 0 && (total - headerSize) % descriptorSize != 0);
    if (implausible)
        total = 0xFFFE;
    if (total % 2) // not all drives like odd allocation lengths
        ++total;
    total = std::min(total, 0xFFFE);

    QByteArray buffer(total, '\0');
    putBe16(cdb, allocOffset, quint16(total));
    r = device.execute(view(cdb, cdbLength), buffer.data(), buffer.size(), Scsi::Direction::FromDevice);
    if (!r.dataUsable())
        return r;

    int received = r.transferred > 0 ? std::min(r.transferred, total) : total;
    if (received >= lengthOffset + 2)
        received = std::min(received, int(be16(buffer, lengthOffset)) + lengthOffset + 2);
    buffer.truncate(received);
    response = buffer;
    return r;
}

}

// ---------------------------------------------------------------------------

Scsi::Result testUnitReady(Scsi::Device &device)
{
    Cdb cdb{};
    cdb[0] = char(OpTestUnitReady);
    return device.execute(view(cdb, 6), nullptr, 0, Scsi::Direction::None, 10000);
}

Scsi::Result inquiry(Scsi::Device &device, QByteArray &response)
{
    Cdb cdb{};
    cdb[0] = char(OpInquiry);
    cdb[4] = char(96);
    response = QByteArray(96, '\0');
    Scsi::Result r = device.execute(view(cdb, 6), response.data(), response.size(), Scsi::Direction::FromDevice, 10000);
    if (r.dataUsable() && r.transferred > 0)
        response.truncate(r.transferred);
    return r;
}

Scsi::Result modeSense10(Scsi::Device &device, quint8 page, QByteArray &response)
{
    Cdb cdb{};
    cdb[0] = char(OpModeSense10);
    cdb[1] = 0x08; // DBD: no block descriptors (not every drive honours it)
    cdb[2] = char(page & 0x3F); // current values
    return readWithLengthHeader(device, cdb, 10, 7, 8, 0, 0, response);
}

Scsi::Result readTocPmaAtip(Scsi::Device &device, quint8 format, bool msf, quint8 trackOrSession, QByteArray &response)
{
    Cdb cdb{};
    cdb[0] = char(OpReadTocPmaAtip);
    cdb[1] = msf ? 0x02 : 0x00;
    cdb[2] = char(format & 0x0F);
    cdb[6] = char(trackOrSession);

    int descriptorSize = 0;
    switch (format & 0x0F) {
    case 0:
    case 1:
        descriptorSize = 8;
        break;
    case 2:
        descriptorSize = 11;
        break;
    case 5:
        descriptorSize = 18;
        break;
    default:
        break;
    }
    return readWithLengthHeader(device, cdb, 10, 7, 4, 0, descriptorSize, response);
}

Scsi::Result getConfigurationHeader(Scsi::Device &device, QByteArray &response)
{
    Cdb cdb{};
    cdb[0] = char(OpGetConfiguration);
    cdb[1] = 0x02; // only the feature named in the starting feature number (0: profile list)
    response = QByteArray(8, '\0');
    putBe16(cdb, 7, quint16(response.size()));
    const Scsi::Result r = device.execute(view(cdb, 10), response.data(), int(response.size()), Scsi::Direction::FromDevice, 10000);
    if (!r.dataUsable() || r.transferred < response.size())
        response.clear(); // a short answer would look like a zero profile
    return r;
}

Scsi::Result readCd(Scsi::Device &device, int lba, int count, bool c2, char *buffer, int bufferLength, bool subQ, int timeoutMs)
{
    Cdb cdb{};
    cdb[0] = char(OpReadCd);
    cdb[1] = 0x04; // expected sector type: CD-DA
    const quint32 address = quint32(lba); // two's complement for lead-in addresses
    cdb[2] = char(address >> 24);
    cdb[3] = char(address >> 16);
    cdb[4] = char(address >> 8);
    cdb[5] = char(address);
    cdb[6] = char(count >> 16);
    cdb[7] = char(count >> 8);
    cdb[8] = char(count);
    cdb[9] = char(0x10 | (c2 ? 0x02 : 0x00)); // user data (+ C2 error flags)
    cdb[10] = subQ ? 0x02 : 0x00; // formatted Q sub-channel or none
    return device.execute(view(cdb, 12), buffer, bufferLength, Scsi::Direction::FromDevice, timeoutMs);
}

Scsi::Result readCdRawSubchannel(Scsi::Device &device, int lba, int count, char *buffer, int bufferLength, int timeoutMs)
{
    Cdb cdb{};
    cdb[0] = char(OpReadCd);
    cdb[1] = 0x04; // expected sector type: CD-DA
    const quint32 address = quint32(lba); // two's complement for lead-in addresses
    cdb[2] = char(address >> 24);
    cdb[3] = char(address >> 16);
    cdb[4] = char(address >> 8);
    cdb[5] = char(address);
    cdb[6] = char(count >> 16);
    cdb[7] = char(count >> 8);
    cdb[8] = char(count);
    cdb[9] = 0x10; // user data
    cdb[10] = 0x01; // raw P-W sub-channel
    return device.execute(view(cdb, 12), buffer, bufferLength, Scsi::Direction::FromDevice, timeoutMs);
}

Scsi::Result setCdSpeed(Scsi::Device &device, int readKbps)
{
    Cdb cdb{};
    cdb[0] = char(OpSetCdSpeed);
    putBe16(cdb, 2, quint16(std::clamp(readKbps, 0, 0xFFFF)));
    putBe16(cdb, 4, 0xFFFF); // write speed: leave at maximum
    return device.execute(view(cdb, 12), nullptr, 0, Scsi::Direction::None, 10000);
}

Scsi::Result setStreaming(Scsi::Device &device, int readKbps)
{
    std::array<char, 28> descriptor{}; // performance descriptor
    if (readKbps <= 0) {
        descriptor[0] = 0x04; // RDD: restore the drive's default performance
    } else {
        putBe32(descriptor, 8, 0xFFFFFFFF); // end LBA: up to the end of the medium
        putBe32(descriptor, 12, quint32(readKbps)); // read size (kB) ...
        putBe32(descriptor, 16, 1000); // ... per read time (ms)
        putBe32(descriptor, 20, quint32(readKbps)); // write: the same
        putBe32(descriptor, 24, 1000);
    }
    Cdb cdb{};
    cdb[0] = char(OpSetStreaming);
    putBe16(cdb, 9, quint16(descriptor.size())); // byte 8, type 0: performance descriptor
    return device.execute(view(cdb, 12), descriptor.data(), int(descriptor.size()), Scsi::Direction::ToDevice, 10000);
}

Scsi::Result preventMediumRemoval(Scsi::Device &device, bool prevent)
{
    Cdb cdb{};
    cdb[0] = char(OpPreventAllow);
    cdb[4] = prevent ? 0x01 : 0x00;
    return device.execute(view(cdb, 6), nullptr, 0, Scsi::Direction::None, 10000);
}

// ---------------------------------------------------------------------------

bool waitUntilReady(Scsi::Device &device, int timeoutMs, QString *error)
{
    QElapsedTimer timer;
    timer.start();
    Scsi::Result r;
    while (true) {
        r = testUnitReady(device);
        if (r.ok())
            return true;
        const bool becomingReady = r.sense.valid && r.sense.key == 0x02 && r.sense.asc == 0x04 && r.sense.ascq == 0x01;
        const bool unitAttention = r.sense.valid && r.sense.key == 0x06;
        if (!(becomingReady || unitAttention) || timer.elapsed() > timeoutMs)
            break;
        QThread::msleep(becomingReady ? 500 : 50);
    }
    if (error)
        *error = u"Drive not ready: %1"_s.arg(r.toString());
    return false;
}

std::optional<Cdda::Toc> readToc(Scsi::Device &device, QString *error, QStringList *log)
{
    auto note = [log](const QString &msg) {
        if (log)
            log->append(msg);
    };

    QByteArray raw;
    Scsi::Result r = readTocPmaAtip(device, 2, true, 1, raw);
    if (r.dataUsable()) {
        QString parseError;
        if (const auto entries = parseFullToc(raw)) {
            if (auto toc = tocFromFullToc(*entries, &parseError))
                return toc;
        } else {
            parseError = u"malformed response (%1 bytes)"_s.arg(raw.size());
        }
        note(u"Full TOC unusable: %1"_s.arg(parseError));
    } else {
        note(u"READ TOC (full TOC) failed: %1"_s.arg(r.toString()));
    }

    r = readTocPmaAtip(device, 0, false, 1, raw);
    if (!r.dataUsable()) {
        if (error)
            *error = u"READ TOC failed: %1"_s.arg(r.toString());
        return std::nullopt;
    }
    const auto entries = parseFormattedToc(raw);
    if (!entries) {
        if (error)
            *error = u"Malformed TOC response (%1 bytes)"_s.arg(raw.size());
        return std::nullopt;
    }
    return tocFromFormattedToc(*entries, error);
}

std::optional<InquiryData> readInquiry(Scsi::Device &device, QString *error)
{
    QByteArray raw;
    const Scsi::Result r = inquiry(device, raw);
    if (!r.dataUsable()) {
        if (error)
            *error = u"INQUIRY failed: %1"_s.arg(r.toString());
        return std::nullopt;
    }
    auto data = parseInquiry(raw);
    if (!data && error)
        *error = u"INQUIRY returned only %1 bytes"_s.arg(raw.size());
    return data;
}

std::optional<Capabilities> readCapabilities(Scsi::Device &device, QString *error)
{
    QByteArray raw;
    const Scsi::Result r = modeSense10(device, 0x2A, raw);
    if (!r.dataUsable()) {
        if (error)
            *error = u"MODE SENSE (page 2Ah) failed: %1"_s.arg(r.toString());
        return std::nullopt;
    }
    auto caps = parseCapabilitiesPage(raw);
    if (!caps && error)
        *error = u"Capabilities page is malformed (%1 bytes)"_s.arg(raw.size());
    return caps;
}

}
