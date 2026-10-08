/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QStringList>

#include <optional>

#include "core/mmcparse.h"
#include "core/toc.h"
#include "scsidevice.h"

// Thin MMC command layer: builds CDBs, handles allocation lengths and hands
// the raw responses to the pure parsers in core/mmcparse.h.

namespace Audex::Mmc
{

Scsi::Result testUnitReady(Scsi::Device &device);
Scsi::Result inquiry(Scsi::Device &device, QByteArray &response);
Scsi::Result modeSense10(Scsi::Device &device, quint8 page, QByteArray &response);
Scsi::Result readTocPmaAtip(Scsi::Device &device, quint8 format, bool msf, quint8 trackOrSession, QByteArray &response);
// GET CONFIGURATION, only the 8 byte header with the current profile
Scsi::Result getConfigurationHeader(Scsi::Device &device, QByteArray &response);

// READ CD for CD-DA sectors. With c2 the drive appends 294 bytes of C2 error
// flags to every sector, with subQ 16 bytes of formatted Q sub-channel after
// that. buffer must hold count * (2352 [+ 294] [+ 16]) bytes.
Scsi::Result readCd(Scsi::Device &device, int lba, int count, bool c2, char *buffer, int bufferLength, bool subQ = false, int timeoutMs = 30000);

// READ CD for CD-DA sectors with the raw P-W sub-channel (96 bytes) after
// the 2352 bytes of each sector.
Scsi::Result readCdRawSubchannel(Scsi::Device &device, int lba, int count, char *buffer, int bufferLength, int timeoutMs = 30000);

// kB/s, 0xFFFF = maximum. Usually requires the device to be opened read/write.
Scsi::Result setCdSpeed(Scsi::Device &device, int readKbps);
// SET STREAMING for reading, kB/s; 0 restores the drive's default (maximum).
// Many newer drives ignore SET CD SPEED and only follow this one.
Scsi::Result setStreaming(Scsi::Device &device, int readKbps);
Scsi::Result preventMediumRemoval(Scsi::Device &device, bool prevent);

// --- convenience ---

// Polls TEST UNIT READY while the drive reports "becoming ready".
bool waitUntilReady(Scsi::Device &device, int timeoutMs, QString *error);

// Full TOC with fallback to the formatted TOC.
std::optional<Cdda::Toc> readToc(Scsi::Device &device, QString *error, QStringList *log = nullptr);

std::optional<InquiryData> readInquiry(Scsi::Device &device, QString *error);
std::optional<Capabilities> readCapabilities(Scsi::Device &device, QString *error);

}
