/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QHash>
#include <QImage>
#include <QMap>
#include <QMetaType>
#include <QSet>
#include <QString>
#include <QVariant>

typedef QMap<quint32, QVariant> DatasetMap;
typedef QSet<quint32> TypeSet;

class Dataset
{
public:
    Dataset();
    Dataset(const Dataset &other);
    Dataset &operator=(const Dataset &other);

    bool operator==(const Dataset &other) const;
    bool operator!=(const Dataset &other) const;

    const QVariant get(const quint32 type, const QVariant &def = QVariant()) const;
    bool set(const quint32 type, const QVariant &value);
    bool contains(const quint32 type) const;
    bool remove(const quint32 type, const bool withSubsets = false);

    const Dataset &subset(const int index) const;
    Dataset &subset(const int index);
    Dataset &operator[](const int index);
    const Dataset &firstSubset() const;
    Dataset &firstSubset();
    const Dataset &lastSubset() const;
    Dataset &lastSubset();
    int appendSubset(const Dataset &subset); // return index of appended subset
    int subsetCount() const;

    const QVariant getCustom(const QString &label, const QVariant &def = QVariant()) const;
    bool setCustom(const QString &label, const QVariant &value);
    bool customContains(const QString &label);
    bool removeCustom(const QString &label);

    void merge(const Dataset &other);

    bool isModified(const bool withSubsets = true) const;
    bool isModified(const quint32 type) const;
    // check if a set of labels was modified
    bool isModified(const TypeSet &types) const;

    bool isValid(const quint32 type) const;

    // flush all data
    void clear();
    // data will not be indicated as modified any longer
    void confirm();

    bool isEmpty() const;

    // if read only there is no modification possible
    // can be bypassed by accessing data()
    inline void setReadOnly(const bool ro = true)
    {
        p_readonly = ro;
    }
    inline bool isReadOnly() const
    {
        return p_readonly;
    }

    void swap(const uint type1, const uint type2);

    // some basic string handling methods
    // splits stype by divider into dtype1 (part before divider) and dtype2 (part after divider)
    void split(const uint type, const QString &divider, const uint dtype1, const uint dtype2);
    void capitalize(const uint type);

    inline DatasetMap *data()
    {
        return &p_data;
    }
    inline QMap<QString, QVariant> *customData()
    {
        return &p_custom_data;
    }

protected:
    DatasetMap p_data;
    QMap<QString, QVariant> p_custom_data;

    QList<Dataset> p_subsets;
    bool p_subset_length_modified;

    QHash<uint, bool> p_data_modified;
    void p_modify(const uint type);

    bool p_readonly;

    const QString p_capitalize(const QString &string);
};

Q_DECLARE_METATYPE(Dataset)
