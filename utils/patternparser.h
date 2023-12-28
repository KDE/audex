/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PATTERNPARSER_H
#define PATTERNPARSER_H

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QObject>
#include <QString>
#include <QXmlDefaultHandler>

#include <KLocalizedString>

#include "config.h"

#define IS_TRUE(val) (((val.toLower() == "true") || (val == "1") || (val.toLower() == "on")) ? true : false)

#define VAR_FILENAME_PATTERN "filenamepattern"
#define VAR_COMMAND_PATTERN "commandpattern"
#define VAR_SIMPLE_PATTERN "simplepattern"
#define VAR_TEXT_PATTERN "textpattern"

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
#define STANDARD_EMBED_NOCOVER_PLACEHOLDER_WIDTH 300

class SaxHandler : public QXmlDefaultHandler
{
public:
    SaxHandler();
    ~SaxHandler() override;

    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) override;
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override;
    bool characters(const QString &ch) override;
    bool fatalError(const QXmlParseException &exception) override;

    void setInputFile(const QString &input)
    {
        this->input = input;
    }
    void setOutputFile(const QString &output)
    {
        this->output = output;
    }
    void setTrackNo(const int trackno)
    {
        this->trackno = trackno;
    }
    void setCDNo(const int cdno)
    {
        this->cdno = cdno;
    }
    void setTrackOffset(const int trackoffset)
    {
        this->trackoffset = trackoffset;
    }
    void setArtist(const QString &artist)
    {
        this->artist = artist;
    }
    void setTitle(const QString &title)
    {
        this->title = title;
    }
    void setTrackArtist(const QString &tartist)
    {
        this->tartist = tartist;
    }
    void setTrackTitle(const QString &ttitle)
    {
        this->ttitle = ttitle;
    }
    void setDate(const QString &date)
    {
        this->date = date;
    }
    void setGenre(const QString &genre)
    {
        this->genre = genre;
    }
    void setSuffix(const QString &suffix)
    {
        this->suffix = suffix;
    }
    void setCover(const QImage &cover)
    {
        this->cover = cover;
    }
    void setFAT32Compatible(const bool fat32compatible)
    {
        this->fat32compatible = fat32compatible;
    }
    void setReplaceSpacesWithUnderscores(const bool replacespaceswithunderscores)
    {
        this->replacespaceswithunderscores = replacespaceswithunderscores;
    }
    void set2DigitsTrackNum(const bool _2digitstracknum)
    {
        this->_2digitstracknum = _2digitstracknum;
    }
    void setTMPPath(const QString &tmppath)
    {
        this->tmppath = tmppath;
    }
    void setDiscid(const quint32 discid)
    {
        this->discid = discid;
    }
    void setSize(const qreal size)
    {
        this->size = size;
    }
    void setLength(const int length)
    {
        this->length = length;
    }
    void setNoOfTracks(const int nooftracks)
    {
        this->nooftracks = nooftracks;
    }
    void setDemoMode(const bool demomode)
    {
        this->demomode = demomode;
    }
    void setEncoder(const QString &encoder)
    {
        this->encoder = encoder;
    }

    inline const QString text() const
    {
        return p_text;
    }

private:
    QString pattern;
    QString input;
    QString output;
    int trackno;
    int cdno;
    int trackoffset;
    QString artist;
    QString title;
    QString tartist;
    QString ttitle;
    QString date;
    QString genre;
    QString suffix;
    QImage cover;
    bool fat32compatible;
    bool replacespaceswithunderscores;
    bool _2digitstracknum;
    QString tmppath;
    quint32 discid;
    qreal size;
    int length;
    int nooftracks;
    QString encoder;

    bool demomode;
    /*TEMP*/ bool found_suffix;

    QString p_text;
    QString p_element;

    bool is_filename_pattern;
    bool is_command_pattern;
    bool is_simple_pattern;
    bool is_text_pattern;

    const QString make_compatible(const QString &string);
    const QString make_compatible_2(const QString &string);
    const QString make_fat32_compatible(const QString &string);
    const QString replace_spaces_with_underscores(const QString &string);
    const QString replace_char_list(const QXmlAttributes &atts, const QString &string);
};

class PatternParser : public QObject
{
    Q_OBJECT
public:
    explicit PatternParser(QObject *parent = nullptr);
    ~PatternParser() override;

    const QString parseFilenamePattern(const QString &pattern,
                                       int trackno,
                                       int cdno,
                                       int trackoffset,
                                       int nooftracks,
                                       const QString &artist,
                                       const QString &title,
                                       const QString &tartist,
                                       const QString &ttitle,
                                       const QString &date,
                                       const QString &genre,
                                       const QString &suffix,
                                       bool fat32compatible,
                                       bool replacespaceswithunderscores,
                                       bool _2digitstracknum);

    const QString parseCommandPattern(const QString &pattern,
                                      const QString &input,
                                      const QString &output,
                                      int trackno,
                                      int cdno,
                                      int trackoffset,
                                      int nooftracks,
                                      const QString &artist,
                                      const QString &title,
                                      const QString &tartist,
                                      const QString &ttitle,
                                      const QString &date,
                                      const QString &genre,
                                      const QString &suffix,
                                      const QImage &cover,
                                      bool fat32compatible,
                                      const QString &tmppath,
                                      const QString &encoder,
                                      const bool demomode = false);

    const QString parseSimplePattern(const QString &pattern,
                                     int cdno,
                                     int nooftracks,
                                     const QString &artist,
                                     const QString &title,
                                     const QString &date,
                                     const QString &genre,
                                     const QString &suffix,
                                     bool fat32compatible);

    void parseInfoText(QStringList &text,
                       const QString &artist,
                       const QString &title,
                       const QString &date,
                       const QString &genre,
                       const quint32 discid,
                       const qreal size,
                       const int length,
                       const int nooftracks);

Q_SIGNALS:
    void error(const QString &message, const QString &details = QString());

private:
    const QString p_xmlize_pattern(const QString &pattern);
};

#endif
