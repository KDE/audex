/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "dataset.h"

Dataset::Dataset()
{
    p_readonly = false;
    p_subset_length_modified = false;
}

Dataset::Dataset(const Dataset &other)
{
    p_data = other.p_data;
    p_custom_data = other.p_custom_data;
    p_data_modified = other.p_data_modified;

    p_subsets = other.p_subsets;
    p_subset_length_modified = other.p_subset_length_modified;

    p_readonly = other.p_readonly;
}

Dataset &Dataset::operator=(const Dataset &other)
{
    p_data = other.p_data;
    p_custom_data = other.p_custom_data;
    p_data_modified = other.p_data_modified;

    p_subsets = other.p_subsets;
    p_subset_length_modified = other.p_subset_length_modified;

    p_readonly = other.p_readonly;

    return *this;
}

bool Dataset::operator==(const Dataset &other) const
{
    if (p_data != other.p_data)
        return false;
    if (p_custom_data != other.p_custom_data)
        return false;

    for (int i = 0; i < p_subsets.count(); ++i)
        if (p_subsets.at(i) != other.p_subsets.at(i))
            return false;

    return true;
}

bool Dataset::operator!=(const Dataset &other) const
{
    return !(*this == other);
}

const QVariant Dataset::get(const quint32 type, const QVariant &def) const
{
    return p_data.value(type, def);
}

bool Dataset::set(const quint32 type, const QVariant &value)
{
    if ((value == get(type)) || (p_readonly))
        return false;
    p_data.insert(type, value);
    p_modify(type);
    return true;
}

bool Dataset::contains(const quint32 type) const
{
    return p_data.contains(type);
}

bool Dataset::remove(const quint32 type, const bool withSubsets)
{
    bool result = p_data.remove(type) > 0;

    if (withSubsets)
        for (int i = 0; i < p_subsets.count(); ++i)
            if (p_subsets[i].remove(type))
                result = true;

    return result;
}

const Dataset &Dataset::subset(const int index) const
{
    return p_subsets.at(index);
}

Dataset &Dataset::subset(const int index)
{
    while (index >= p_subsets.count())
        appendSubset(Dataset());
    return p_subsets[index];
}

Dataset &Dataset::operator[](const int index)
{
    return subset(index);
}

const Dataset &Dataset::firstSubset() const
{
    return p_subsets.first();
}

Dataset &Dataset::firstSubset()
{
    return p_subsets.first();
}

const Dataset &Dataset::lastSubset() const
{
    return p_subsets.last();
}

Dataset &Dataset::lastSubset()
{
    return p_subsets.last();
}

int Dataset::appendSubset(const Dataset &subset)
{
    if (p_readonly)
        return -1;
    p_subsets.append(subset);
    p_subset_length_modified = true;
    return p_subsets.length() - 1;
}

int Dataset::subsetCount() const
{
    return p_subsets.count();
}

const QVariant Dataset::getCustom(const QString &label, const QVariant &def) const
{
    return p_custom_data.value(label, def);
}

bool Dataset::setCustom(const QString &label, const QVariant &value)
{
    if ((value == getCustom(label)) || p_readonly)
        return false;
    p_custom_data.insert(label, value);
    return true;
}

bool Dataset::customContains(const QString &label)
{
    return p_custom_data.contains(label);
}

bool Dataset::removeCustom(const QString &label)
{
    return p_custom_data.remove(label) > 0;
}

void Dataset::merge(const Dataset &other)
{
    Dataset otherDataset = other;

    for (auto i = otherDataset.data()->begin(), end = otherDataset.data()->end(); i != end; ++i)
        set(i.key(), i.value());

    for (auto i = otherDataset.customData()->begin(), end = otherDataset.customData()->end(); i != end; ++i)
        setCustom(i.key(), i.value());

    for (int i = 0; i < otherDataset.subsetCount(); ++i)
        subset(i).merge(other.p_subsets.at(i));
}

bool Dataset::isModified(const bool withSubsets) const
{
    if (p_subset_length_modified)
        return true;

    QHash<quint32, bool>::const_iterator i = p_data_modified.constBegin();
    while (i != p_data_modified.constEnd()) {
        if (i.value())
            return true;
        ++i;
    }

    if (withSubsets)
        for (int i = 0; i < p_subsets.count(); ++i)
            if (p_subsets.at(i).isModified())
                return true;

    return false;
}

bool Dataset::isModified(const quint32 type) const
{
    if (p_data_modified.value(type, false))
        return true;

    return false;
}

bool Dataset::isModified(const TypeSet &types) const
{
    TypeSet::ConstIterator i = types.constBegin();
    while (i != types.constEnd())
        if (isModified(*i))
            return true;

    return false;
}

bool Dataset::isValid(const quint32 type) const
{
    QVariant value = p_data.value(type, QVariant());
    if (!value.isValid())
        return false;
#if QT_VERSION >= 0x060000
    if (value.typeId() == QMetaType::QString)
#else
    if (value.type() == QVariant::String)
#endif
        return !value.toString().isEmpty();
#if QT_VERSION >= 0x060000
    if (value.typeId() == QMetaType::QImage)
#else
    if (value.type() == QVariant::Image)
#endif
        return !value.value<QImage>().isNull();
    else
        return true;
}

void Dataset::clear()
{
    if (p_readonly)
        return;
    p_data.clear();
    p_custom_data.clear();
    p_data_modified.clear();
    p_subsets.clear();
    p_subset_length_modified = false;
}

void Dataset::confirm()
{
    p_data_modified.clear();
    for (int i = 0; i < p_subsets.count(); ++i)
        p_subsets[i].confirm();
    p_subset_length_modified = false;
}

bool Dataset::isEmpty() const
{
    return p_data.count() == 0 && p_custom_data.count() == 0 && p_subsets.count() == 0;
}

void Dataset::swap(const quint32 type1, const quint32 type2)
{
    if (p_readonly)
        return;
    if (type1 == type2)
        return;

    QVariant tmp = get(type1);
    set(type1, get(type2));
    set(type2, tmp);
}

void Dataset::split(const quint32 stype, const QString &divider, const quint32 dtype1, const quint32 dtype2)
{
    if (p_readonly)
        return;
    if (divider.isEmpty())
        return;

#if QT_VERSION >= 0x060000
    if (get(stype).typeId() == QMetaType::QString)
#else
    if (get(stype).type() != QVariant::String)
#endif
        return;

    int splitPos = get(stype).toString().indexOf(divider);
    if (splitPos >= 0) {
        // split
        QString part1 = get(stype).toString().mid(splitPos + divider.length());
        QString part2 = get(stype).toString().left(splitPos);
        set(dtype1, part1);
        set(dtype2, part2);
    }
}

void Dataset::capitalize(const quint32 type)
{
    if (p_readonly)
        return;
#if QT_VERSION >= 0x060000
    if (get(type).typeId() == QMetaType::QString)
#else
    if (get(type).type() != QVariant::String)
#endif
        return;

    set(type, p_capitalize(get(type).toString()));
}

void Dataset::p_modify(const quint32 type)
{
    p_data_modified.insert(type, true);
}

const QString Dataset::p_capitalize(const QString &string)
{
    QStringList stringlist = string.split(u' ', Qt::SkipEmptyParts);
    for (int i = 0; i < stringlist.count(); ++i) {
        QString string = stringlist[i].toLower();
        int j = 0;
        while (((string[j] == u'(') || (string[j] == u'[' || (string[j] == u'{'))) && (j < string.length()))
            j++;
        string[j] = string[j].toUpper();
        stringlist[i] = string;
    }

    return stringlist.join(u' ');
}
