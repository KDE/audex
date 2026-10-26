/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "test_framework.h"

#include "utils/ripjob.h"

using namespace Audex;
using namespace Qt::StringLiterals;

AUDEX_TEST("rip status: independent phases and overall result")
{
    PhaseResult extraction{JobStatus::Success};
    PhaseResult verification{JobStatus::Skipped};
    PhaseResult output{JobStatus::Success};
    AUDEX_CHECK(t, overallStatus(extraction, verification, output) == JobStatus::Success);

    verification.status = JobStatus::Warning;
    AUDEX_CHECK(t, overallStatus(extraction, verification, output) == JobStatus::Warning);
    extraction.status = JobStatus::Warning;
    AUDEX_CHECK(t, overallStatus(extraction, verification, output) == JobStatus::Warning);
    output.status = JobStatus::Failed;
    AUDEX_CHECK(t, overallStatus(extraction, verification, output) == JobStatus::Failed);
    extraction.status = JobStatus::Canceled;
    AUDEX_CHECK(t, overallStatus(extraction, verification, output) == JobStatus::Canceled);
}

AUDEX_TEST("rip status: verification distinctions")
{
    RipSummary summary;
    summary.completed = true;
    summary.verifiableTracks = 3;
    summary.verifiedTracks = 3;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Skipped);

    summary.verificationRequested = true;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Success);
    summary.verifiedTracks = 2;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Warning);
    summary.verifiedTracks = 3;
    summary.accurateRipMismatches = 1;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Warning);
    summary.accurateRipMismatches = 0;
    summary.ctdbError = u"Network error"_s;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Warning);
    summary.ctdbError.clear();
    AUDEX_CHECK(t, verificationStatus(summary, true, true) == JobStatus::Warning);
    AUDEX_CHECK(t, verificationStatus(summary, false, false) == JobStatus::Skipped);
    summary.repairCheckFailed = true;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Failed);
    summary.repairCheckFailed = false;
    summary.verifiableTracks = 0;
    AUDEX_CHECK(t, verificationStatus(summary, true, false) == JobStatus::Skipped);
}
