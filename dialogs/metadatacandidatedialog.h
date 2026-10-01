/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDialog>

#include "metadata/candidate.h"

class QListWidget;
class QDialogButtonBox;

// Lets the user pick one of the metadata candidates (like the old libkcddb
// multiple match dialog).
class MetadataCandidateDialog : public QDialog
{
    Q_OBJECT

public:
    MetadataCandidateDialog(const Audex::MetadataCandidates &candidates, QWidget *parent = nullptr);

    int selectedCandidate() const; // index into the candidate list, -1 if none

private:
    QListWidget *list = nullptr;
    QDialogButtonBox *buttonBox = nullptr;
};
