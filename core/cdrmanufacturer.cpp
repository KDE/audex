/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2009-2013 Thomas Schmitt <scdbackup@gmx.net>
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * CD-R/RW manufacturer by the lead-in start of the ATIP, taken from libburn
 * (libburnia-project.org, libburn/util.c, burn_guess_cd_manufacturer()):
 * the table of codes and names and the lookup rule (frame rounded down to
 * a multiple of 10). libburn is provided under GPL version 2 or later; the
 * full license text is in LICENSES/GPL-2.0-or-later.txt.
 */

#include "cdrmanufacturer.h"

namespace Audex::Mmc
{

QString cdrManufacturer(int minute, int second, int frame)
{
    struct Code {
        const char *name;
        int minute;
        int second;
        int frame;
    };
    static const Code codes[] = {
        {"SKC", 96, 40, 0},
        {"Ritek Corp", 96, 43, 30},
        {"TDK / Ritek", 97, 10, 0},
        {"TDK Corporation", 97, 15, 0},
        {"Ritek Corp", 97, 15, 10},
        {"Mitsubishi Chemical Corporation", 97, 15, 20},
        {"Nan-Ya Plastics Corporation", 97, 15, 30},
        {"Delphi", 97, 15, 50},
        {"Shenzhen SG&SAST", 97, 16, 20},
        {"Moser Baer India Limited", 97, 17, 0},
        {"SKY media Manufacturing SA", 97, 17, 10},
        {"Wing", 97, 18, 10},
        {"DDT", 97, 18, 20},
        {"Daxon Technology Inc. / Acer", 97, 22, 60},
        {"Taiyo Yuden Company Limited", 97, 24, 0},
        {"Sony Corporation", 97, 24, 10},
        {"Computer Support Italcard s.r.l", 97, 24, 20},
        {"Unitech Japan Inc.", 97, 24, 30},
        {"MPO, France", 97, 25, 0},
        {"Hitachi Maxell Ltd.", 97, 25, 20},
        {"Infodisc Technology Co,Ltd.", 97, 25, 30},
        {"Xcitec", 97, 25, 60},
        {"Fornet International Pte Ltd", 97, 26, 0},
        {"Postech Corporation", 97, 26, 10},
        {"SKC Co Ltd.", 97, 26, 20},
        {"Fuji Photo Film Co,Ltd.", 97, 26, 40},
        {"Lead Data Inc.", 97, 26, 50},
        {"CMC Magnetics Corporation", 97, 26, 60},
        {"Ricoh Company Limited", 97, 27, 0},
        {"Plasmon Data Systems Ltd", 97, 27, 10},
        {"Princo Corporation", 97, 27, 20},
        {"Pioneer", 97, 27, 30},
        {"Eastman Kodak Company", 97, 27, 40},
        {"Mitsui Chemicals Inc.", 97, 27, 50},
        {"Ricoh Company Limited", 97, 27, 60},
        {"Gigastorage Corporation", 97, 28, 10},
        {"Multi Media Masters&Machinary SA", 97, 28, 20},
        {"Ritek Corp", 97, 31, 0},
        {"Grand Advance Technology Sdn. Bhd.", 97, 31, 30},
        {"TDK Corporation", 97, 32, 0},
        {"Prodisc Technology Inc.", 97, 32, 10},
        {"Mitsubishi Chemical Corporation", 97, 34, 20},
        {"Mitsui Chemicals Inc.", 97, 48, 50},
        {"TDK Corporation", 97, 49, 0},
    };
    const int tens = frame - frame % 10;
    for (const Code &c : codes)
        if (c.minute == minute && c.second == second && (c.frame == tens || c.frame == frame))
            return QString::fromLatin1(c.name);
    return QString();
}

}
