/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QMap>
#include <QProcess>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QVariant>

#include <KLocalizedString>

#include "encoding/registry.h"
#include "utils/encodercommand.h"
#include "metadata/cdinfo.h"
#include "models/cdinfomodel.h"
#include "models/profilemodel.h"
#include "utils/encoderassistant.h"
#include "utils/ripjob.h"

// Translates an Audex profile (ProfileModel), the disc model (CDInfoModel)
// and the application settings (Preferences) into a RipRequest for the
// extraction engine, plus a plan for the additional files the profile asks
// for (cover file, playlist, info, cue sheet, log).

struct PostProcessPlan {
    bool imageFile = false;

    bool saveCover = false;
    QString coverFilePath;
    bool coverScale = false;
    QSize coverSize;
    QString coverFormat; // "JPEG" or "PNG"

    bool writePlaylist = false;
    QString playlistFilePath;
    bool playlistAbsolutePaths = false;
    bool playlistUtf8 = true;

    bool writeCue = false;
    QString cueFilePath;
    bool cueMcnIsrc = false;

    bool runHook = false;
    QString hookCommand; // raw scheme; placeholders are filled at run time
    QMap<QString, QString> hookVars; // album values for the scheme
    QString outputDir;
};

class RipRequestBuilder
{
public:
    RipRequestBuilder(ProfileModel *profileModel, Audex::CDInfoModel *cdInfoModel, std::shared_ptr<const Audex::Encoding::EncoderRegistry> encoders);

    void setDrive(const Audex::DriveEntry &drive);
    void setDriveUdi(const QString &udi); // Solid UDI: selects the per-drive read settings

    // Checks selection and output directory. existingFiles receives the
    // target files that already exist (overwrite confirmation is up to the
    // caller, see Preferences::overwriteExistingFiles()).
    bool validate(QString *error, QStringList *existingFiles = nullptr) const;

    Audex::RipRequest request() const;
    PostProcessPlan plan() const;

    // Space the rip needs in the output folder, estimated from the selected
    // audio (its WAVE size for lossless and custom encoders, a third of it
    // for MP3 and Opus), and what the file system has free (-1: unknown).
    struct DiskSpace {
        QString folder;
        qint64 needed = 0;
        qint64 available = -1;
        bool enough() const
        {
            return available < 0 || available >= needed;
        }
    };
    DiskSpace diskSpace() const;

    // The drive does not read accurately (no "accurate stream"): its reads
    // are a few samples off after every seek, which Audex does not realign.
    // Secure rips then flag almost every sector, fast rips are shifted.
    // Measured by the drive test, or, untested, as the drive reports it.
    struct StreamWarning {
        bool warn = false;
        bool measured = false; // by the drive test; otherwise reported by the drive
        int jitterSamples = 0; // measured
    };
    StreamWarning streamWarning() const;

    QString outputDirectory() const; // folder of the (first) audio file
    bool isImageFile() const;

    // Audio file per selected track, built with SchemeParser from the profile's
    // name scheme. Image file rips map all tracks to the same path. Empty on error.
    QMap<int, QString> filePaths(QString *error = nullptr) const;

    // Executes the plan after a successfully finished rip (GUI thread).
    // log(level, text) receives Rip::LogLevel-compatible levels.
    static void runPostProcess(const PostProcessPlan &plan,
                               const Audex::RipSummary &summary,
                               const Audex::CDInfo &disc,
                               const QList<int> &tracks,
                               QObject *parent,
                               const std::function<void(int, const QString &)> &log);

private:
    QVariant columnVariant(int col) const; // value of the current profile row
    QString column(int col) const;
    bool columnBool(int col) const;

    void encoder(QString *id, QVariantMap *settings, QString *suffix) const;

    // Encoder command of the profile, translated for the engine; what the
    // command cannot use lands in `issues` as one entry per problem.
    QStringList externalCommand(const EncoderAssistant::Encoder encoder,
                                const Parameters &parameters,
                                QList<Audex::Encoding::CommandIssue> *issues = nullptr) const;

    QMap<QString, QString> albumVars() const; // values a scheme can fill in
    QString sanitizePathSegment(const QString &name) const;
    bool replaceSpaces() const; // "Replace spaces with underscores", track rips only
    bool fat32() const; // "Create FAT32 compatible filenames", track rips only

    // A file name scheme: its values are made safe for a path segment, the
    // result is relative to the folder it is written to and has the suffix.
    // Empty with `error` set if the scheme cannot be used.
    QString fileNameValue(const QString &value) const;
    Placeholders fileNameValues(const QString &suffix) const; // album values and $suffix
    QString fileName(const QString &scheme, const Placeholders &values, const QString &suffix, QString *error = nullptr) const;
    QString resolveNameScheme(const QString &scheme, const QString &suffix) const; // cover/playlist/... names
    QString logFilePath() const; // rip log of the profile, empty if it writes none
    bool wantsCdg() const; // .cdg files wanted, and the disc may carry graphics

    ProfileModel *profile_model;
    Audex::CDInfoModel *cdda_model;
    std::shared_ptr<const Audex::Encoding::EncoderRegistry> m_encoders;
    Audex::DriveEntry m_drive;
    QString m_driveUdi;
    QString m_application;
};
