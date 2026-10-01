/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// CUETools database (CTDB, db.cuetools.net): verification of a whole disc
// and of single tracks against the CRC-32 checksums other users submitted.
// Definitions follow the CUETools sources (CUETools.CTDB, CUETools.AccurateRip).

#include "core/ctdbparity.h"
#include "core/ripengine.h"
#include "core/toc.h"

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

#include <functional>

namespace Audex::Ctdb
{

// The disc CRC leaves out the first StrideSamples of the first audio track
// and the last skipLast() samples (10 to 20 sectors) of the last one
// (StrideSamples and ShiftRange: core/ctdbparity.h).
int skipLast(const Cdda::Toc &toc);

// start sectors (index 01) of all tracks, data tracks marked with '-', then
// the lead-out: "0:15200:-40000:52000"
QString tocString(const Cdda::Toc &toc);
QString tocId(const Cdda::Toc &toc); // as CUETools shows it in the log
QUrl lookupUrl(const Cdda::Toc &toc);

struct Entry {
    qint64 id = 0;
    quint32 crc = 0; // disc CRC
    int confidence = 0;
    int stride = 0; // samples
    int npar = 0; // parity words per stride (repair)
    QList<quint16> syndromes; // of the first column (up to MaxParity), to find the shift
    QString parityUrl; // recovery data; empty if there is none
    QString toc;
    QList<quint32> trackCrcs; // per audio track; empty for old submissions

    bool hasParity() const
    {
        return !parityUrl.isEmpty();
    }
};

QList<Entry> parseResponse(const QByteArray &xml, QString *error = nullptr);

// The entry describes a disc with the same audio (length and CRC windows)
bool isSameAudio(const Entry &entry, const Cdda::Toc &toc);

// Downloads the recovery data of an entry: the syndromes of all columns for
// entry.syndromes.size() parity words (Parity::syndromeData() layout)
QByteArray fetchParity(const Entry &entry, QString *error, int timeoutMs = 60000, const std::function<bool()> &isCanceled = {});

// Synchronous lookup for worker threads. An empty list without error means
// the disc is not in the database.
QList<Entry> lookup(const Cdda::Toc &toc, QString *error, int timeoutMs = 30000, const std::function<bool()> &isCanceled = {});

// Confidence with which the database confirms a single ripped track: the
// summed confidence of the entries with the same audio whose track CRC matches
// at the read offset or at a shift of another pressing found via AccurateRip
// (SegmentResult::accurateRipShifted). Unlike formatVerification() there is
// no search over all shifts: one track alone is too little for that.
int trackConfidence(const QList<Entry> &entries, const Rip::SegmentResult &track, const Cdda::Toc &toc);
// Some entry can confirm single tracks (same audio, with track CRCs)
bool canConfirmTracks(const QList<Entry> &entries, const Cdda::Toc &toc);

// Compares the CTDB CRCs computed while ripping (SegmentResult::ctdbCrc, the
// shifted variants and, with RipOptions::ctdbShiftRange, the CRCs at every
// shift derived from SegmentResult::ctdbHead/ctdbTail) with the database entries and
// formats the outcome as log lines, the first line being the section heading.
// mismatches receives the number of tracks the database contradicts.
QStringList formatVerification(const QList<Entry> &entries, const QList<Rip::SegmentResult> &segments, const Cdda::Toc &toc, int *mismatches = nullptr);

}
