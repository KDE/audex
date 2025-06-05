/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdparanoia.h"

namespace Audex
{

namespace Device
{

Cdparanoia::Cdparanoia(const QByteArray &blockDevice, const bool enableParanoiaMode, const int maxRetriesOnReadError)
{
    block_device = blockDevice;

    if (enableParanoiaMode)
        paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP;
    else
        paranoia_mode = PARANOIA_MODE_DISABLE;

    init();

    paranoia_max_retries = maxRetriesOnReadError;
}

bool Cdparanoia::init()
{
    drive = cdda_identify(block_device.constData(), 0, nullptr);
    if (!drive) {
        qDebug() << "Failed to find device.";
        return false;
    }

    if (cdda_open(drive) != 0) {
        qDebug() << "Failed to open disc.";
        return false;
    }

    paranoia = paranoia_init(drive);
    if (!paranoia) {
        qDebug() << "Failed to create cdparanoia object.";
        return false;
    }

    paranoia_modeset(paranoia, paranoia_mode);

    return true;
}

Cdparanoia::~Cdparanoia()
{
    if (paranoia) {
        paranoia_free(paranoia);
        paranoia = nullptr;
    }
    if (drive) {
        cdda_close(drive);
        drive = nullptr;
    }
}

void Cdparanoia::enableParanoiaMode(const bool enable)
{
    if (enable)
        paranoia_mode = PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP; // full paranoia mode, but allow skipping
    else
        paranoia_mode = PARANOIA_MODE_DISABLE;

    if (paranoia)
        paranoia_modeset(paranoia, paranoia_mode);
}

bool Cdparanoia::paranoiaModeEnabled() const
{
    return paranoia_mode != PARANOIA_MODE_DISABLE;
}

void Cdparanoia::setParanoiaMaxRetriesOnReadError(int max_retries)
{
    paranoia_max_retries = max_retries;
}

int Cdparanoia::paranoiaMaxRetriesOnReadError() const
{
    return paranoia_max_retries;
}

qint16 *Cdparanoia::paranoiaRead(void (*callback)(long, paranoia_cb_mode_t))
{
    if (paranoia)
        return cdio_paranoia_read_limited(paranoia, callback, paranoia_max_retries);
    return nullptr;
}

int Cdparanoia::paranoiaSeek(const int sector, qint32 mode)
{
    if (paranoia)
        return (int)cdio_paranoia_seek(paranoia, sector, mode);
    return -1;
}

bool Cdparanoia::paranoiaError(QString &errorMsg)
{
    errorMsg.clear();
    if (drive) {
        char *msg = cdio_cddap_errors(drive);
        if (msg) {
            errorMsg = QString::fromLocal8Bit(msg, -1);
            cdio_cddap_free_messages(msg);
            return true;
        }
    }
    return false;
}

}

}
