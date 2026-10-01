/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "fields.h"

#include <array>

using namespace Qt::StringLiterals;

namespace Audex::Metadata
{

namespace
{

using enum Field;
using T = FieldType;

constexpr std::array<FieldInfo, int(FieldCount)> fieldTable{{
    {Artist, T::Text, "ARTIST"},
    {ArtistSort, T::Text, "ARTISTSORT"},
    {Composer, T::Text, "COMPOSER"},
    {Songwriter, T::Text, "SONGWRITER"},
    {Arranger, T::Text, "ARRANGER"},
    {Performer, T::Text, "PERFORMER"},
    {Album, T::Text, "ALBUM"},
    {Title, T::Text, "TITLE"},
    {DiscSubtitle, T::Text, "DISCSUBTITLE"},
    {Work, T::Text, "WORK"},
    {Genre, T::Text, "GENRE"},
    {Year, T::Text, "DATE"},
    {Comment, T::Text, "COMMENT"},
    {Label, T::Text, "LABEL"},
    {CatalogNumber, T::Text, "CATALOGNUMBER"},
    {Barcode, T::Text, "BARCODE"},
    {MCN, T::Text, "MCN"},
    {ISRC, T::Text, "ISRC"},
    {Country, T::Text, "RELEASECOUNTRY"},
    {CddbCategory, T::Text, "CDDB_CATEGORY"},
    {CddbDiscId, T::Text, "CDDB_DISCID"},
    {MusicBrainzDiscId, T::Text, "MUSICBRAINZ_DISCID"},
    {MusicBrainzReleaseId, T::Text, "MUSICBRAINZ_ALBUMID"},
    {MusicBrainzReleaseGroupId, T::Text, "MUSICBRAINZ_RELEASEGROUPID"},
    {MusicBrainzRecordingId, T::Text, "MUSICBRAINZ_TRACKID"},
    {MusicBrainzTrackId, T::Text, "MUSICBRAINZ_RELEASETRACKID"},
    {MusicBrainzArtistId, T::Text, "MUSICBRAINZ_ARTISTID"},
    {DiscNumber, T::Number, "DISCNUMBER"},
    {DiscCount, T::Number, "DISCTOTAL"},
    {TrackNumberOffset, T::Number, "TRACKNUMBEROFFSET"},
    {VariousArtists, T::Flag, "COMPILATION"},
    {MultiDisc, T::Flag, "MULTIDISC"},
}};

// Compile time check that the table order matches the enum
constexpr bool tableIsOrdered()
{
    for (int i = 0; i < int(FieldCount); ++i)
        if (int(fieldTable[i].field) != i)
            return false;
    return true;
}
static_assert(tableIsOrdered(), "fieldTable must list the fields in enum order");

bool isEmptyValue(const QVariant &v)
{
    return !v.isValid() || v.isNull();
}

}

const FieldInfo &fieldInfo(Field field)
{
    Q_ASSERT(int(field) < int(FieldCount));
    return fieldTable[int(field)];
}

QList<Field> allFields()
{
    QList<Field> result;
    for (const FieldInfo &i : fieldTable)
        result.append(i.field);
    return result;
}

QString tagName(Field field)
{
    return QString::fromLatin1(fieldInfo(field).tag);
}

// ---------------------------------------------------------------------------

QVariant Fields::normalized(Field field, const QVariant &value)
{
    if (isEmptyValue(value))
        return QVariant();
    switch (fieldInfo(field).type) {
    case FieldType::Text: {
        const QString s = value.toString();
        return s.trimmed().isEmpty() ? QVariant() : QVariant(s);
    }
    case FieldType::Number: {
        bool ok = false;
        const int n = value.toInt(&ok);
        return (!ok || n == 0) ? QVariant() : QVariant(n);
    }
    case FieldType::Flag:
        return value.toBool() ? QVariant(true) : QVariant();
    }
    return QVariant();
}

bool Fields::contains(Field field) const
{
    return m_values.contains(field);
}

QVariant Fields::value(Field field) const
{
    return m_values.value(field);
}

QString Fields::text(Field field) const
{
    return m_values.value(field).toString();
}

int Fields::number(Field field) const
{
    return m_values.value(field).toInt();
}

bool Fields::flag(Field field) const
{
    return m_values.value(field).toBool();
}

bool Fields::setValue(Field field, const QVariant &value)
{
    const QVariant v = normalized(field, value);
    if (!v.isValid())
        return remove(field);
    const auto it = m_values.constFind(field);
    if (it != m_values.cend() && it.value() == v)
        return false;
    m_values.insert(field, v);
    m_modified.insert(int(field));
    return true;
}

bool Fields::setText(Field field, const QString &value)
{
    return setValue(field, value);
}

bool Fields::setNumber(Field field, int value)
{
    return setValue(field, value);
}

bool Fields::setFlag(Field field, bool value)
{
    return setValue(field, value);
}

bool Fields::remove(Field field)
{
    if (m_values.remove(field) == 0)
        return false;
    m_modified.insert(int(field));
    return true;
}

QList<Field> Fields::fields() const
{
    return m_values.keys();
}

bool Fields::containsCustom(const QString &key) const
{
    return m_custom.contains(key);
}

QVariant Fields::custom(const QString &key) const
{
    return m_custom.value(key);
}

bool Fields::setCustom(const QString &key, const QVariant &value)
{
    if (isEmptyValue(value) || (value.typeId() == QMetaType::QString && value.toString().isEmpty()))
        return removeCustom(key);
    const auto it = m_custom.constFind(key);
    if (it != m_custom.cend() && it.value() == value)
        return false;
    m_custom.insert(key, value);
    m_modifiedCustom.insert(key);
    return true;
}

bool Fields::removeCustom(const QString &key)
{
    if (m_custom.remove(key) == 0)
        return false;
    m_modifiedCustom.insert(key);
    return true;
}

QStringList Fields::customKeys() const
{
    return m_custom.keys();
}

bool Fields::isEmpty() const
{
    return m_values.isEmpty() && m_custom.isEmpty();
}

void Fields::clear()
{
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it)
        m_modified.insert(int(it.key()));
    for (auto it = m_custom.cbegin(); it != m_custom.cend(); ++it)
        m_modifiedCustom.insert(it.key());
    m_values.clear();
    m_custom.clear();
}

void Fields::reset()
{
    m_values.clear();
    m_custom.clear();
    confirm();
}

bool Fields::isModified() const
{
    return !m_modified.isEmpty() || !m_modifiedCustom.isEmpty();
}

bool Fields::isModified(Field field) const
{
    return m_modified.contains(int(field));
}

bool Fields::isModified(std::initializer_list<Field> fields) const
{
    for (Field f : fields)
        if (isModified(f))
            return true;
    return false;
}

bool Fields::isCustomModified(const QString &key) const
{
    return m_modifiedCustom.contains(key);
}

void Fields::confirm()
{
    m_modified.clear();
    m_modifiedCustom.clear();
}

bool Fields::merge(const Fields &other, MergePolicy policy)
{
    bool changed = false;
    for (auto it = other.m_values.cbegin(); it != other.m_values.cend(); ++it) {
        if (policy == MergePolicy::KeepExisting && contains(it.key()))
            continue;
        changed |= setValue(it.key(), it.value());
    }
    for (auto it = other.m_custom.cbegin(); it != other.m_custom.cend(); ++it) {
        if (policy == MergePolicy::KeepExisting && containsCustom(it.key()))
            continue;
        changed |= setCustom(it.key(), it.value());
    }
    return changed;
}

bool Fields::swap(Field a, Field b)
{
    if (a == b)
        return false;
    const QVariant va = value(a);
    const QVariant vb = value(b);
    bool changed = setValue(a, vb);
    changed |= setValue(b, va);
    return changed;
}

bool Fields::split(Field source, const QString &divider, Field before, Field after)
{
    if (divider.isEmpty() || fieldInfo(source).type != FieldType::Text)
        return false;
    const QString s = text(source);
    const qsizetype pos = s.indexOf(divider);
    if (pos < 0)
        return false;
    const QString head = s.left(pos).trimmed();
    const QString tail = s.mid(pos + divider.size()).trimmed();
    // write in an order that works if source is one of the targets
    bool changed = false;
    if (before != source && after != source)
        changed |= remove(source);
    changed |= setText(before, head);
    changed |= setText(after, tail);
    return changed;
}

bool Fields::capitalize(Field field)
{
    if (fieldInfo(field).type != FieldType::Text || !contains(field))
        return false;
    return setText(field, capitalized(text(field)));
}

bool Fields::operator==(const Fields &other) const
{
    return m_values == other.m_values && m_custom == other.m_custom;
}

// ---------------------------------------------------------------------------

QString capitalized(const QString &text)
{
    QStringList words = text.split(u' ', Qt::SkipEmptyParts);
    for (QString &word : words) {
        word = word.toLower();
        qsizetype j = 0;
        while (j < word.size() && !word.at(j).isLetterOrNumber())
            ++j;
        if (j < word.size())
            word[j] = word.at(j).toUpper();
    }
    return words.join(u' ');
}

QDebug operator<<(QDebug debug, const Fields &fields)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Fields(";
    bool first = true;
    for (Field f : fields.fields()) {
        debug << (first ? "" : ", ") << tagName(f) << '=' << fields.value(f);
        first = false;
    }
    for (const QString &k : fields.customKeys()) {
        debug << (first ? "" : ", ") << k << '=' << fields.custom(k);
        first = false;
    }
    debug << ')';
    return debug;
}

}
