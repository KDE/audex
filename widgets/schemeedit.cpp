/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "schemeedit.h"

#include "utils/encodercommand.h"
#include "utils/schemeparser.h"

#include <KLocalizedString>

#include <QAction>
#include <QCursor>
#include <QDate>
#include <QIcon>
#include <QMenu>
#include <QTime>
#include <QToolTip>

namespace Var = Audex::Scheme::Var;

namespace
{

// values standing in for the album variables when a scheme is checked
QMap<QString, QString> exampleAlbumValues()
{
    return {{Var::AlbumArtist, QStringLiteral("Meat Loaf")},
            {Var::AlbumTitle, QStringLiteral("Bat Out Of Hell III")},
            {Var::Date, QStringLiteral("2006")},
            {Var::Genre, QStringLiteral("Rock")},
            {Var::CdNo, QString()},
            {Var::NoOfTracks, QStringLiteral("12")},
            {Var::Encoder, QStringLiteral("LAME 3.100")},
            {Var::Application, QStringLiteral("Audex")},
            {Var::DiscId, QStringLiteral("a70de90c")},
            {Var::Mcn, QStringLiteral("4006381333931")},
            {Var::CdSize, QStringLiteral("587 MiB")},
            {Var::CdLength, QStringLiteral("55:42.120")},
            {Var::Today, QDate::currentDate().toString(Qt::ISODate)},
            {Var::Now, QTime::currentTime().toString(QStringLiteral("hh-mm-ss"))}};
}

// the values of a track filename scheme
QMap<QString, QString> exampleTrackValues()
{
    QMap<QString, QString> values = exampleAlbumValues();
    values.insert(Var::TrackArtist, QStringLiteral("Meat Loaf"));
    values.insert(Var::TrackTitle, QStringLiteral("Blind As A Bat"));
    values.insert(Var::TrackNo, QStringLiteral("02"));
    values.insert(Var::Isrc, QStringLiteral("AA6Q72000047"));
    return values;
}

}

SchemeEdit::SchemeEdit(QWidget *parent)
    : QLineEdit(parent)
{
    QAction *insertAction = addAction(QIcon::fromTheme(QStringLiteral("list-add")), QLineEdit::TrailingPosition);
    insertAction->setToolTip(i18n("Insert placeholder"));
    connect(insertAction, &QAction::triggered, this, [this]() {
        if (m_insertMenu)
            m_insertMenu->exec(QCursor::pos());
    });

    m_warningAction = addAction(QIcon::fromTheme(QStringLiteral("dialog-warning")), QLineEdit::TrailingPosition);
    m_warningAction->setVisible(false);

    QAction *helpAction = addAction(QIcon::fromTheme(QStringLiteral("help-contents")), QLineEdit::TrailingPosition);
    helpAction->setToolTip(i18n("Help"));
    connect(helpAction, &QAction::triggered, this, &SchemeEdit::showHelp);

    connect(this, &QLineEdit::textChanged, this, &SchemeEdit::validate);
    connect(this, &QLineEdit::textEdited, this, &SchemeEdit::edited);

    applyKind();
}

SchemeEdit::Kind SchemeEdit::kind() const
{
    return m_kind;
}

void SchemeEdit::setKind(Kind kind)
{
    if (m_kind == kind)
        return;
    m_kind = kind;
    applyKind();
}

QString SchemeEdit::scheme() const
{
    return text();
}

void SchemeEdit::setScheme(const QString &scheme)
{
    setText(scheme);
    setCursorPosition(0);
}

void SchemeEdit::applyKind()
{
    setPlaceholderText(m_kind == Command || m_kind == HookCommand ? i18n("Please set a command") : i18n("Please set a scheme"));
    rebuildMenu();
    validate();
}

void SchemeEdit::insertText(const QString &text)
{
    setFocus();
    insert(text);
    Q_EMIT edited();
}

void SchemeEdit::rebuildMenu()
{
    if (!m_insertMenu)
        m_insertMenu = new QMenu(this);
    m_insertMenu->clear();

    // an entry carries the text it inserts, the label shows it in brackets
    const auto addEntry = [this](const QString &label, const QString &text) {
        m_insertMenu->addAction(label + QStringLiteral(" (") + text + QLatin1Char(')'), this, [this, text]() {
            insertText(text);
        });
    };
    const auto addVariable = [&addEntry](const QString &label, const QString &name) {
        addEntry(label, QStringLiteral("$") + name);
    };

    m_insertMenu->addSection(i18n("Album"));
    addVariable(i18n("Album Artist"), Var::AlbumArtist);
    addVariable(i18n("Album Title"), Var::AlbumTitle);
    addVariable(i18n("Date"), Var::Date);
    addVariable(i18n("Genre"), Var::Genre);
    addVariable(i18n("CD #"), Var::CdNo);
    addVariable(i18n("# of Tracks"), Var::NoOfTracks);
    addVariable(i18n("Today"), Var::Today);

    if (m_kind != Filename && m_kind != HookCommand) {
        m_insertMenu->addSection(i18n("Track"));
        addVariable(i18n("Track Artist"), Var::TrackArtist);
        addVariable(i18n("Track Title"), Var::TrackTitle);
        addVariable(i18n("Track #"), Var::TrackNo);
    }

    if (m_kind == Command) {
        m_insertMenu->addSection(i18n("Command"));
        addVariable(i18n("Audio Input"), Var::InputFile);
        addVariable(i18n("Output File"), Var::OutputFile);
    } else if (m_kind == HookCommand) {
        m_insertMenu->addSection(i18n("Hook"));
        addVariable(i18n("Written Files"), Var::HookFiles);
        addVariable(i18n("Output Directory"), Var::HookOutputDir);
    } else {
        m_insertMenu->addSection(i18n("File"));
        addVariable(i18n("Suffix"), Var::Suffix);
    }
}

void SchemeEdit::validate()
{
    QString warning;

    const QString schemeText = text();
    if (!schemeText.isEmpty()) {
        if (m_kind == Command) {
            const Audex::Encoding::CommandScheme command = Audex::Encoding::parseCommandScheme(schemeText, exampleAlbumValues());
            if (!command.issues.isEmpty()) {
                QStringList messages;
                for (const Audex::Encoding::CommandIssue &issue : command.issues)
                    messages << SchemeParser::commandIssueText(issue);
                warning = messages.join(u'\n');
            }
        } else if (m_kind == HookCommand) {
            const QList<Audex::Encoding::CommandIssue> issues = Audex::Encoding::checkHookCommand(schemeText, exampleAlbumValues());
            if (!issues.isEmpty()) {
                QStringList messages;
                for (const Audex::Encoding::CommandIssue &issue : issues)
                    messages << SchemeParser::commandIssueText(issue);
                warning = messages.join(u'\n');
            }
        } else {
            Placeholders values = m_kind == TrackFilename ? exampleTrackValues() : exampleAlbumValues();
            values.insert(Var::Suffix, QStringLiteral("flac"));
            SchemeParser parser;
            parser.parseScheme(schemeText, values);
            warning = parser.error() ? parser.errorString() : parser.warnings().join(u'\n');
        }
    }

    m_warningAction->setVisible(!warning.isEmpty());
    m_warningAction->setToolTip(warning);
}

void SchemeEdit::showHelp()
{
    int document = 3;
    if (m_kind == TrackFilename)
        document = 1;
    else if (m_kind == Command)
        document = 2;
    else if (m_kind == HookCommand)
        document = 4;
    // an empty rectangle keeps the tooltip open when the mouse moves away
    QToolTip::showText(QCursor::pos(), SchemeParser::helpHTMLDoc(document), this, QRect(), 60000);
}
