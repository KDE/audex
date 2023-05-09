/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WAVEFILEWRITER_HEADER
#define WAVEFILEWRITER_HEADER

#include <QDataStream>
#include <QFile>
#include <QString>

#include <KLocalizedString>

class WaveFileWriter : public QObject
{
    Q_OBJECT
public:
    WaveFileWriter();
    ~WaveFileWriter() override;

    enum Endianess { BigEndian, LittleEndian };

    /**
     * open a new wave file.
     * closes any opened file.
     */
    bool open(const QString &filename);
    bool isOpen();
    QString filename() const;

    void setEndianess(const Endianess e);
    Endianess endianess();

    /**
     * closes the file.
     * Length of the wave file will be written into the header.
     * If no data has been written to the file except the header
     * it will be removed.
     */
    void close();

public Q_SLOTS:
    /**
     * write 16bit samples to the file.
     */
    void write(const QByteArray &data);

Q_SIGNALS:
    void error(const QString &errorstr);

private:
    void p_write_empty_header();
    void p_update_header();

    Endianess p_endianess;

    QFile p_output_file;
    QDataStream p_output_stream;
    QString p_filename;

    long p_data_written;
};

#endif
