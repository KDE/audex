/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Successor of the generic Dataset class, now dedicated to metadata:
//  - strongly typed keys (enum class Field) instead of quint32 shared between
//    unrelated enums (Toc::Type, Metadata::Type, DriveInfo::Type ...)
//  - normalized values: empty text, 0 and false mean "not set", so contains()
//    is meaningful and comparisons do not depend on QVariant types
//  - modification tracking covers set, remove and custom fields
//  - no subsets: the album owns its tracks explicitly (see metadata.h)
//  - value semantics without hand written copy operations

#include <QDebug>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <initializer_list>

namespace Audex::Metadata
{

enum class Field : quint8 {
    // --- text ---
    Artist, // album: album artist, track: track artist
    ArtistSort,
    Composer,
    Songwriter,
    Arranger,
    Performer,
    Album,
    Title, // track title (album title is Field::Album)
    DiscSubtitle, // title of the medium within a multi disc release
    Work,
    Genre,
    Year, // "YYYY" (a full date "YYYY-MM-DD" is accepted as well)
    Comment, // may contain line breaks
    Label,
    CatalogNumber,
    Barcode, // UPC/EAN of the release (database)
    MCN, // media catalog number as read from the disc
    ISRC,
    Country,
    CddbCategory,
    CddbDiscId,
    MusicBrainzDiscId,
    MusicBrainzReleaseId,
    MusicBrainzReleaseGroupId,
    MusicBrainzRecordingId,
    MusicBrainzTrackId,
    MusicBrainzArtistId,

    // --- numbers ---
    DiscNumber,
    DiscCount,
    TrackNumberOffset, // added to the displayed/tagged track numbers

    // --- flags ---
    VariousArtists,
    MultiDisc,

    FieldCount
};

enum class FieldType { Text, Number, Flag };

struct FieldInfo {
    Field field;
    FieldType type;
    const char *tag; // tag name, Vorbis comment style (Picard naming where it exists)
};

const FieldInfo &fieldInfo(Field field);
QList<Field> allFields();
QString tagName(Field field);

enum class MergePolicy {
    Overwrite, // values of the other set replace existing ones
    KeepExisting, // only fill fields that are not set yet
};

class Fields
{
public:
    bool contains(Field field) const;
    QVariant value(Field field) const;
    QString text(Field field) const;
    int number(Field field) const;
    bool flag(Field field) const;

    // All setters return true if the stored value changed. Empty text, 0 and
    // false remove the field.
    bool setValue(Field field, const QVariant &value);
    bool setText(Field field, const QString &value);
    bool setNumber(Field field, int value);
    bool setFlag(Field field, bool value);
    bool remove(Field field);
    QList<Field> fields() const;

    // Free form fields, e.g. tags without a Field or provider specific data
    bool containsCustom(const QString &key) const;
    QVariant custom(const QString &key) const;
    bool setCustom(const QString &key, const QVariant &value); // invalid or empty removes
    bool removeCustom(const QString &key);
    QStringList customKeys() const;

    bool isEmpty() const;
    void clear(); // remove all values; removals count as modifications
    void reset(); // remove all values and forget modifications

    bool isModified() const;
    bool isModified(Field field) const;
    bool isModified(std::initializer_list<Field> fields) const;
    bool isCustomModified(const QString &key) const;
    void confirm(); // current state becomes the unmodified state

    bool merge(const Fields &other, MergePolicy policy = MergePolicy::Overwrite);

    bool swap(Field a, Field b);
    // "before<divider>after": source is split at the first divider, parts are trimmed
    bool split(Field source, const QString &divider, Field before, Field after);
    bool capitalize(Field field);

    // Compares content only, not the modification state
    bool operator==(const Fields &other) const;
    bool operator!=(const Fields &other) const
    {
        return !(*this == other);
    }

private:
    static QVariant normalized(Field field, const QVariant &value);

    QMap<Field, QVariant> m_values;
    QMap<QString, QVariant> m_custom;
    QSet<int> m_modified;
    QSet<QString> m_modifiedCustom;
};

// "the WALL (live)" -> "The Wall (Live)"; words are separated by spaces,
// leading brackets and quotes are skipped.
QString capitalized(const QString &text);

QDebug operator<<(QDebug debug, const Fields &fields);

}
