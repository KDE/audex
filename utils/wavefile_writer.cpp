/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2024 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "wavefile_writer.h"

namespace Audex
{

WAVEFileWriter::WAVEFileWriter()
{
    p_data_written = 0;
    p_endianess = LittleEndian;
}

WAVEFileWriter::~WAVEFileWriter()
{
    close();
}

bool WAVEFileWriter::open(const QString &filename)
{
    close();

    p_data_written = 0;

    p_output_file.setFileName(filename);
    if (p_output_file.open(QIODevice::WriteOnly)) {
        p_filename = filename;
        p_write_empty_header();
        return true;
    } else {
        return false;
    }
}

bool WAVEFileWriter::isOpen()
{
    return p_output_file.isOpen();
}

QString WAVEFileWriter::filename() const
{
    return p_filename;
}

void WAVEFileWriter::setEndianess(const Endianess e)
{
    p_endianess = e;
}

WAVEFileWriter::Endianess WAVEFileWriter::endianess()
{
    return p_endianess;
}

void WAVEFileWriter::close()
{
    if (isOpen()) {
        if (p_data_written) {
            // update wave header
            p_update_header();
            p_output_file.close();
        } else {
            p_output_file.close();
            p_output_file.remove();
        }
    }
    p_filename.clear();
}

void WAVEFileWriter::write(const QByteArray &data)
{
    int len = data.size();
    if (isOpen()) {
        if (p_endianess == LittleEndian) {
            qint64 ret = p_output_file.write(data);
            if (ret == -1) {
                Q_EMIT log(Message(p_output_file.errorString(), Message::CRITICAL));
                return;
            }
        } else {
            if (data.size() % 2 > 0) {
                qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__ << "Data length is not a multiple of 2! Cannot write data.";
                Q_EMIT log(Message(i18n("Unable to write WAVE file. Internal buffer length error."), Message::CRITICAL));
                return;
            }
            // we need to swap the bytes
            char *buffer = new char[len];
            for (int i = 0; i < len - 1; i += 2) {
                buffer[i] = data.data()[i + 1];
                buffer[i + 1] = data.data()[i];
            }
            p_output_file.write(buffer, len);
            delete[] buffer;
        }
        p_data_written += len;
    }
}

void WAVEFileWriter::p_write_empty_header()
{
    static const unsigned char riffHeader[] = {
        0x52, 0x49, 0x46, 0x46, // 0  "RIFF"
        0x00, 0x00, 0x00, 0x00, // 4  wavSize
        0x57, 0x41, 0x56, 0x45, // 8  "WAVE"
        0x66, 0x6d, 0x74, 0x20, // 12 "fmt "
        0x10, 0x00, 0x00, 0x00, // 16
        0x01, 0x00, 0x02, 0x00, // 20
        0x44, 0xac, 0x00, 0x00, // 24
        0x10, 0xb1, 0x02, 0x00, // 28
        0x04, 0x00, 0x10, 0x00, // 32
        0x64, 0x61, 0x74, 0x61, // 36 "data"
        0x00, 0x00, 0x00, 0x00 // 40 byteCount
    };
    p_output_file.write((char *)riffHeader, 44);
}

void WAVEFileWriter::p_update_header()
{
    if (isOpen()) {
        p_output_file.flush();

        char c[4];
        qint32 wavSize = p_data_written + 44 - 8;

        // jump to the wavSize position in the header

        p_output_file.seek(4);
        c[0] = (wavSize >> 0) & 0xff;
        c[1] = (wavSize >> 8) & 0xff;
        c[2] = (wavSize >> 16) & 0xff;
        c[3] = (wavSize >> 24) & 0xff;
        p_output_file.write(c, 4);

        p_output_file.seek(40);
        c[0] = (p_data_written >> 0) & 0xff;
        c[1] = (p_data_written >> 8) & 0xff;
        c[2] = (p_data_written >> 16) & 0xff;
        c[3] = (p_data_written >> 24) & 0xff;
        p_output_file.write(c, 4);

        // jump back to the end
        p_output_file.seek(p_output_file.size());
    }
}

}
