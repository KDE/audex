/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PLACEHOLDERPARSER_H
#define PLACEHOLDERPARSER_H

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMetaType>
#include <QObject>

#include <KLocalizedString>

#include "config.h"
#include "utils/parameters.h"

#define VAR_ALBUM_ARTIST "artist"
#define VAR_ALBUM_TITLE "title"
#define VAR_TRACK_ARTIST "tartist"
#define VAR_TRACK_TITLE "ttitle"
#define VAR_TRACK_NO "trackno"
#define VAR_CD_NO "cdno"
#define VAR_DATE "date"
#define VAR_GENRE "genre"
#define VAR_SUFFIX "suffix"
#define VAR_ENCODER "encoder"

#define VAR_INPUT_FILE "i"
#define VAR_OUTPUT_FILE "o"
#define VAR_COVER_FILE "cover"

#define VAR_DISCID "discid"
#define VAR_CD_SIZE "size"
#define VAR_CD_LENGTH "length"
#define VAR_TODAY "today"
#define VAR_NOW "now"
#define VAR_LINEBREAK "br"

#define VAR_AUDEX "audex"
#define VAR_NO_OF_TRACKS "nooftracks"

#define STANDARD_EMBED_COVER_FORMAT "jpeg"
#define STANDARD_EMBED_COVER_WIDTH 600

typedef QMap<QString, QVariant> Placeholders;
typedef QMap<QString, Parameters> PlaceholdersParameters;

class SchemeParser : public QObject
{
    Q_OBJECT
public:
    explicit SchemeParser(QObject *parent = nullptr);
    ~SchemeParser() override;

    // placeholders_parameters: return the actually found placeholders with their parameters as QMap
    const QString parseScheme(const QString &scheme, const Placeholders &placeholders, PlaceholdersParameters *placeholders_parameters = nullptr);

    const QString parseFilenameScheme(const QString &scheme,
                                       const int trackno,
                                       const int cdno,
                                       const int trackoffset,
                                       const int nooftracks,
                                       const QString &artist,
                                       const QString &title,
                                       const QString &tartist,
                                       const QString &ttitle,
                                       const QString &date,
                                       const QString &genre,
                                       const QString &suffix,
                                       const bool fat32compatible,
                                       const bool replacespaceswithunderscores,
                                       const bool twodigitstracknum);

    const QString parseCommandScheme(const QString &scheme,
                                      const QString &input,
                                      const QString &output,
                                      const int trackno,
                                      const int cdno,
                                      const int trackoffset,
                                      const int nooftracks,
                                      const QString &artist,
                                      const QString &title,
                                      const QString &tartist,
                                      const QString &ttitle,
                                      const QString &date,
                                      const QString &genre,
                                      const QString &suffix,
                                      const QImage &cover,
                                      const QString &tmppath,
                                      const QString &encoder,
                                      const bool demomode = false);

    const QString parseSimpleScheme(const QString &scheme,
                                     const int cdno,
                                     const int nooftracks,
                                     const QString &artist,
                                     const QString &title,
                                     const QString &date,
                                     const QString &genre,
                                     const QString &suffix,
                                     const bool fat32compatible);

    void parseInfoTextScheme(QStringList &text,
                              const QString &artist,
                              const QString &title,
                              const QString &date,
                              const QString &genre,
                              const quint32 discid,
                              const qreal size,
                              const int length,
                              const int nooftracks);

    inline bool error() const
    {
        return !p_error_string.isEmpty();
    }

    inline const QString errorString() const
    {
        return p_error_string;
    }

Q_SIGNALS:
    void error(const QString &message, const QString &details = QString());

private:
    const Parameters parse_keyvalue_stringlist(const QStringList &keyvaluepairs);

    QString p_error_string;

    inline const QString
    customize_placeholder_value(const QString &string, bool fat32compatible = false, bool compatible2 = true, bool replacespaceswithunderscores = false)
    {
        QString tmp = string;
        if (fat32compatible)
            tmp = make_fat32_compatible(tmp);
        else if (compatible2)
            tmp = make_compatible_2(tmp);
        else
            tmp = make_compatible(tmp);
        if (replacespaceswithunderscores)
            tmp = replace_spaces_with_underscores(tmp);
        return tmp;
    }

    const QString make_compatible(const QString &string);
    const QString make_compatible_2(const QString &string);
    const QString make_fat32_compatible(const QString &string);
    const QString replace_spaces_with_underscores(const QString &string);
    const QString replace_char_list(const QString &from, const QString &to, const QString &string);
};

#endif
