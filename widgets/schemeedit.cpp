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

namespace
{

// values standing in for the album variables when a scheme is checked
QMap<QString, QString> exampleAlbumValues()
{
    return {{QStringLiteral(VAR_ALBUM_ARTIST), QStringLiteral("Meat Loaf")},
            {QStringLiteral(VAR_ALBUM_TITLE), QStringLiteral("Bat Out Of Hell III")},
            {QStringLiteral(VAR_DATE), QStringLiteral("2006")},
            {QStringLiteral(VAR_GENRE), QStringLiteral("Rock")},
            {QStringLiteral(VAR_CD_NO), QString()},
            {QStringLiteral(VAR_NO_OF_TRACKS), QStringLiteral("12")},
            {QStringLiteral(VAR_ENCODER), QStringLiteral("LAME 3.100")},
            {QStringLiteral(VAR_AUDEX), QStringLiteral("Audex")},
            {QStringLiteral(VAR_DISCID), QStringLiteral("a70de90c")},
            {QStringLiteral(VAR_MCN), QStringLiteral("4006381333931")},
            {QStringLiteral(VAR_CD_SIZE), QStringLiteral("587 MiB")},
            {QStringLiteral(VAR_CD_LENGTH), QStringLiteral("55:42.120")},
            {QStringLiteral(VAR_TODAY), QDate::currentDate().toString(Qt::ISODate)},
            {QStringLiteral(VAR_NOW), QTime::currentTime().toString(QStringLiteral("hh-mm-ss"))},
            {QStringLiteral(VAR_LINEBREAK), QStringLiteral(" ")}};
}

// the values of a track filename scheme
QMap<QString, QString> exampleTrackValues()
{
    QMap<QString, QString> values = exampleAlbumValues();
    values.insert(QStringLiteral(VAR_TRACK_ARTIST), QStringLiteral("Meat Loaf"));
    values.insert(QStringLiteral(VAR_TRACK_TITLE), QStringLiteral("Blind As A Bat"));
    values.insert(QStringLiteral(VAR_TRACK_NO), QStringLiteral("02"));
    values.insert(QStringLiteral(VAR_ISRC), QStringLiteral("AA6Q72000047"));
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
    addVariable(i18n("Album Artist"), QStringLiteral(VAR_ALBUM_ARTIST));
    addVariable(i18n("Album Title"), QStringLiteral(VAR_ALBUM_TITLE));
    addVariable(i18n("Date"), QStringLiteral(VAR_DATE));
    addVariable(i18n("Genre"), QStringLiteral(VAR_GENRE));
    addVariable(i18n("CD #"), QStringLiteral(VAR_CD_NO));
    addVariable(i18n("# of Tracks"), QStringLiteral(VAR_NO_OF_TRACKS));
    addVariable(i18n("Today"), QStringLiteral(VAR_TODAY));

    if (m_kind != Filename && m_kind != HookCommand) {
        m_insertMenu->addSection(i18n("Track"));
        addVariable(i18n("Track Artist"), QStringLiteral(VAR_TRACK_ARTIST));
        addVariable(i18n("Track Title"), QStringLiteral(VAR_TRACK_TITLE));
        addVariable(i18n("Track #"), QStringLiteral(VAR_TRACK_NO));
    }

    if (m_kind == Command) {
        m_insertMenu->addSection(i18n("Command"));
        addVariable(i18n("Audio Input"), QStringLiteral(VAR_INPUT_FILE));
        addVariable(i18n("Output File"), QStringLiteral(VAR_OUTPUT_FILE));
    } else if (m_kind == HookCommand) {
        m_insertMenu->addSection(i18n("Hook"));
        addVariable(i18n("Written Files"), QStringLiteral(VAR_HOOK_FILES));
        addVariable(i18n("Output Directory"), QStringLiteral(VAR_HOOK_OUTPUT_DIR));
    } else {
        m_insertMenu->addSection(i18n("File"));
        addVariable(i18n("Suffix"), QStringLiteral(VAR_SUFFIX));
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
            values.insert(QStringLiteral(VAR_SUFFIX), QStringLiteral("flac"));
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
