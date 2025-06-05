/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include <QString>
#include <QThread>

extern "C" {
#include <unistd.h>
}

#include <Solid/Block>
#include <Solid/Device>

#include <KLocalizedString>

#include "datatypes/metadata.h"

namespace Audex
{

namespace Device
{

struct cdtext_pack {
    unsigned char id1;
    unsigned char id2;
    unsigned char id3;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char dbcc : 1;
    unsigned char blocknum : 3;
    unsigned char charpos : 4;
#else
    unsigned char charpos : 4;
    unsigned char blocknum : 3;
    unsigned char dbcc : 1;
#endif
    unsigned char data[12];
    unsigned char crc[2];
};

class CDTextReader : public QThread
{
    Q_OBJECT

public:
    const QString LastError() const
    {
        return last_error;
    }

public Q_SLOTS:
    void read(const QString &discUDI);

Q_SIGNALS:
    void finished(const QString &driveUDI, const bool successful, const Metadata::Metadata &metadata = Metadata::Metadata());

protected:
    void run() override;

private:
    QString disc_udi;
    QString last_error;
    void debug_raw_text_pack_data(const QByteArray &data);
};

}

}
