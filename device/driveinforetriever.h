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
#include <unistd.h>
}

#include <Solid/Block>
#include <Solid/Device>

#include "datatypes/driveinfo.h"

namespace Audex
{

namespace Device
{

struct inquiry {
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char p_qualifier : 3;
    unsigned char p_device_type : 5;
    unsigned char rmb : 1;
    unsigned char reserved1 : 7;
#else
    unsigned char p_device_type : 5;
    unsigned char p_qualifier : 3;
    unsigned char reserved1 : 7;
    unsigned char rmb : 1;
#endif
    unsigned char version;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char interface_dep : 4;
    unsigned char data_format : 4;
#else
    unsigned char data_format : 4;
    unsigned char interface_dep : 4;
#endif
    unsigned char add_length;
    unsigned char reserved2;
#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
    unsigned char bque : 1;
    unsigned char enc_serv : 1;
    unsigned char vs1 : 1;
    unsigned char multi_p : 1;
    unsigned char m_chngr : 1;
    unsigned char reserved3 : 1;
    unsigned char reserved4 : 1;
    unsigned char addr_16 : 1;
    unsigned char rel_adr : 1;
    unsigned char reserved5 : 1;
    unsigned char w_bus_16 : 1;
    unsigned char sync : 1;
    unsigned char linked : 1;
    unsigned char reserved6 : 1;
    unsigned char cmd_que : 1;
    unsigned char vs2 : 1;
#else
    unsigned char addr_16 : 1;
    unsigned char reserved4 : 1;
    unsigned char reserved3 : 1;
    unsigned char m_chngr : 1;
    unsigned char multi_p : 1;
    unsigned char vs1 : 1;
    unsigned char enc_serv : 1;
    unsigned char bque : 1;
    unsigned char vs2 : 1;
    unsigned char cmd_que : 1;
    unsigned char reserved6 : 1;
    unsigned char linked : 1;
    unsigned char sync : 1;
    unsigned char w_bus_16 : 1;
    unsigned char reserved5 : 1;
    unsigned char rel_adr : 1;
#endif
    unsigned char vendor[8];
    unsigned char model[16];
    unsigned char revision[4];
    unsigned char vendor_specific[20];
    unsigned char reserved7[2];
    unsigned char version1[2];
    unsigned char version2[2];
    unsigned char version3[2];
    unsigned char version4[2];
    unsigned char version5[2];
    unsigned char version6[2];
    unsigned char version7[2];
    unsigned char version8[2];

    // bytes 74-95: reserved
    // bytes 96-n: vendor specific
};

#ifdef WORDS_BIGENDIAN // __BYTE_ORDER == __BIG_ENDIAN
struct cd_wr_speed_performance {
    unsigned char res0; /* Reserved                          */
    unsigned char res_1_27 : 6; /* Reserved                          */
    unsigned char rot_ctl_sel : 2; /* Rotational control selected       */
    unsigned char wr_speed_supp[2]; /* Supported write speed             */
};
#else
struct cd_wr_speed_performance {
    unsigned char res0; /* Reserved                          */
    unsigned char rot_ctl_sel : 2; /* Rotational control selected       */
    unsigned char res_1_27 : 6; /* Reserved                          */
    unsigned char wr_speed_supp[2]; /* Supported write speed             */
};
#endif

struct mm_cap_page_2A {
    unsigned char PS : 1;
    unsigned char res_1 : 1;
    unsigned char page_code : 6;
    unsigned char page_len; /* 0x14 = 20 Bytes (MMC) */
    /* 0x18 = 24 Bytes (MMC-2) */
    /* 0x1C >= 28 Bytes (MMC-3) */
    unsigned char res_2_67 : 2; /* Reserved        */
    unsigned char dvd_ram_read : 1; /* Reads DVD-RAM media       */
    unsigned char dvd_r_read : 1; /* Reads DVD-R media       */
    unsigned char dvd_rom_read : 1; /* Reads DVD ROM media       */
    unsigned char method2 : 1; /* Reads fixed packet method2 media  */
    unsigned char cd_rw_read : 1; /* Reads CD-RW media       */
    unsigned char cd_r_read : 1; /* Reads CD-R  media       */
    unsigned char res_3_67 : 2; /* Reserved        */
    unsigned char dvd_ram_write : 1; /* Supports writing DVD-RAM media    */
    unsigned char dvd_r_write : 1; /* Supports writing DVD-R media      */
    unsigned char res_3_3 : 1; /* Reserved        */
    unsigned char test_write : 1; /* Supports emulation write      */
    unsigned char cd_rw_write : 1; /* Supports writing CD-RW media      */
    unsigned char cd_r_write : 1; /* Supports writing CD-R  media      */
    unsigned char BUF : 1; /* Supports Buffer under. free rec.  */
    unsigned char multi_session : 1; /* Reads multi-session media      */
    unsigned char mode_2_form_2 : 1; /* Reads Mode-2 form 2 media      */
    unsigned char mode_2_form_1 : 1; /* Reads Mode-2 form 1 media (XA)    */
    unsigned char digital_port_1 : 1; /* Supports digital output on port 1 */
    unsigned char digital_port_2 : 1; /* Supports digital output on port 2 */
    unsigned char composite : 1; /* Deliveres composite A/V stream    */
    unsigned char audio_play : 1; /* Supports Audio play operation     */
    unsigned char read_bar_code : 1; /* Supports reading bar codes      */
    unsigned char UPC : 1; /* Reads media catalog number (UPC)  */
    unsigned char ISRC : 1; /* Reads ISRC information      */
    unsigned char c2_pointers : 1; /* Supports C2 error pointers      */
    unsigned char rw_deint_corr : 1; /* Reads de-interleved R-W sub chan  */
    unsigned char rw_supported : 1; /* Reads R-W sub channel information */
    unsigned char cd_da_accurate : 1; /* READ CD data stream is accurate   */
    unsigned char cd_da_supported : 1; /* Reads audio data with READ CD cmd */
    unsigned char loading_type : 3; /* Loading mechanism type      */
    unsigned char res_6_4 : 1; /* Reserved        */
    unsigned char eject : 1; /* Ejects disc/cartr with STOP LoEj  */
    unsigned char prevent_jumper : 1; /* State of prev/allow jumper 0=pres */
    unsigned char lock_state : 1; /* Lock state 0=unlocked 1=locked    */
    unsigned char lock : 1; /* PREVENT/ALLOW may lock media      */
    unsigned char res_7 : 2; /* Reserved        */
    unsigned char rw_in_lead_in : 1; /* Reads raw R-W subcode from lead in */
    unsigned char side_change : 1; /* Side change capable       */
    unsigned char sw_slot_sel : 1; /* Load empty slot in changer      */
    unsigned char disk_present_rep : 1; /* Changer supports disk present rep */
    unsigned char sep_chan_mute : 1; /* Mute controls each channel separately */
    unsigned char sep_chan_vol : 1; /* Vol controls each channel separately */
    unsigned char max_read_speed[2]; /* Max. read speed in KB/s      */
    /* obsolete in MMC-4 */
    unsigned char num_vol_levels[2]; /* # of supported volume levels      */
    unsigned char buffer_size[2]; /* Buffer size for the data in KB    */
    unsigned char cur_read_speed[2]; /* Current read speed in KB/s      */
    /* obsolete in MMC-4 */
    unsigned char res_16; /* Reserved        */
    unsigned char res_17 : 2; /* Reserved        */
    unsigned char length : 2; /* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
    unsigned char LSBF : 1; /* Set: LSB first Clear: MSB first   */
    unsigned char RCK : 1; /* Set: HIGH high LRCK=left channel  */
    unsigned char BCK : 1; /* Data valid on falling edge of BCK */
    unsigned char res_17_0 : 1; /* Reserved        */
    unsigned char max_write_speed[2]; /* Max. write speed supported in KB/s*/
    /* obsolete in MMC-4 */
    unsigned char cur_write_speed[2]; /* Current write speed in KB/s      */
    /* obsolete in MMC-4 */

    /* Byte 22 ... Only in MMC-2      */
    unsigned char copy_man_rev[2]; /* Copy management revision supported*/
    unsigned char res_24; /* Reserved        */
    unsigned char res_25; /* Reserved        */

    /* Byte 26 ... Only in MMC-3      */
    unsigned char res_26; /* Reserved        */
    unsigned char res_27_27 : 6; /* Reserved        */
    unsigned char rot_ctl_sel : 2; /* Rotational control selected      */
    unsigned char v3_cur_write_speed[2]; /* Current write speed in KB/s      */
    unsigned char num_wr_speed_des[2]; /* # of wr speed perf descr. tables  */
    struct cd_wr_speed_performance wr_speed_des[1]; /* wr speed performance descriptor   */
    /* Actually more (num_wr_speed_des)  */
};

const DriveInfo retrieveDriveInfo(const QString &driveUDI, QString &error);

}

}
