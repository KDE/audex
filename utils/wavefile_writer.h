/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WAVEFILE_WRITER_H
#define WAVEFILE_WRITER_H

#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QString>

#include <KLocalizedString>

#include "datatypes/message.h"

namespace Audex
{

class WAVEFileWriter : public QObject
{
    Q_OBJECT
public:
    WAVEFileWriter();
    ~WAVEFileWriter() override;

    enum Endianess { BigEndian, LittleEndian };

    bool open(const QString &filename);
    bool isOpen();
    QString filename() const;

    void setEndianess(const Endianess e);
    Endianess endianess();

public Q_SLOTS:
    // Closes the file.
    // Length of the wave file will be written into the header.
    // If no data has been written to the file except the header
    // it will be removed.
    void close();

    // Write pairs (stereo) of 16bit samples to the file
    void write(const QByteArray &data);

Q_SIGNALS:
    void log(const Message &msg);

private:
    void p_write_empty_header();
    void p_update_header();

    Endianess p_endianess;

    QFile p_output_file;
    QDataStream p_output_stream;
    QString p_filename;

    long p_data_written;
};

}

#endif
