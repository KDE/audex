/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QLineEdit>

class QAction;
class QMenu;

// Line edit for a filename or command scheme with the former wizard built in:
// a menu inside the field inserts placeholders, the help text opens as a
// long-lived tooltip and placeholders Audex cannot fill in are flagged with
// a warning icon inside the field.
class SchemeEdit : public QLineEdit
{
    Q_OBJECT

public:
    enum Kind {
        TrackFilename, // one file name per track
        Filename, // a single file (image, cue sheet, cover, log, playlist)
        Command, // external encoder command
        HookCommand // command hook, run after a rip
    };
    Q_ENUM(Kind)
    Q_PROPERTY(Kind kind READ kind WRITE setKind)

    explicit SchemeEdit(QWidget *parent = nullptr);

    Kind kind() const;
    void setKind(Kind kind);

    QString scheme() const;
    void setScheme(const QString &scheme);

Q_SIGNALS:
    void edited();

private:
    void applyKind();
    void insertText(const QString &text);
    void rebuildMenu();
    void validate();
    void showHelp();

    Kind m_kind = Filename;
    QMenu *m_insertMenu = nullptr;
    QAction *m_warningAction = nullptr;
};
