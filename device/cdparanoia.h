/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDebug>
#include <QString>

#include <KLocalizedString>

extern "C" {
#include <cdio/paranoia/paranoia.h>
}

namespace Audex
{

namespace Device
{

class Cdparanoia
{
public:
    explicit Cdparanoia(const QByteArray &blockDevice, const bool enableParanoiaMode = true, const int maxRetriesOnReadError = 20);
    ~Cdparanoia();

    bool init();

    void enableParanoiaMode(const bool enable = true);
    bool paranoiaModeEnabled() const;
    void setParanoiaMaxRetriesOnReadError(int max_retries); // default: 20
    int paranoiaMaxRetriesOnReadError() const;

    qint16 *paranoiaRead(void (*callback)(long, paranoia_cb_mode_t));
    int paranoiaSeek(const int sector, qint32 mode);
    bool paranoiaError(QString &errorMsg);

private:
    cdrom_drive_t *drive;
    cdrom_paranoia_t *paranoia;

    QByteArray block_device;

    int paranoia_mode;
    int paranoia_max_retries;
};

}

}
