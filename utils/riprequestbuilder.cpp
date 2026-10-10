/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "riprequestbuilder.h"

#include "audex-version.h"
#include "preferences.h"
#include "utils/cuesheetwriter.h"
#include "utils/devicesettings.h"
#include "utils/encoderassistant.h"
#include "utils/encodercommand.h"
#include "utils/playlist.h"
#include "utils/schemeparser.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QTime>

using namespace Qt::StringLiterals;
using Audex::Metadata::Field;

namespace
{

const QList<int> LameBitrates{128, 160, 192, 224, 256, 320};

QString snapLameBitrate(int bitrate)
{
    int best = LameBitrates.first();
    for (const int b : LameBitrates)
        if (qAbs(b - bitrate) < qAbs(best - bitrate))
            best = b;
    return QString::number(best);
}

}

RipRequestBuilder::RipRequestBuilder(ProfileModel *profileModel,
                                     Audex::CDInfoModel *cdInfoModel,
                                     std::shared_ptr<const Audex::Encoding::EncoderRegistry> encoders)
{
    profile_model = profileModel;
    cdda_model = cdInfoModel;
    m_encoders = std::move(encoders);
    m_application = u"Audex "_s + QStringLiteral(AUDEX_VERSION_STRING);
}

void RipRequestBuilder::setDrive(const Audex::DriveEntry &drive)
{
    m_drive = drive;
}

void RipRequestBuilder::setDriveUdi(const QString &udi)
{
    m_driveUdi = udi;
}

QVariant RipRequestBuilder::columnVariant(int col) const
{
    return profile_model->data(profile_model->index(profile_model->currentProfileRow(), col));
}

QString RipRequestBuilder::column(int col) const
{
    return columnVariant(col).toString();
}

bool RipRequestBuilder::columnBool(int col) const
{
    return profile_model->data(profile_model->index(profile_model->currentProfileRow(), col)).toBool();
}

// ---- encoder ----------------------------------------------------------------

void RipRequestBuilder::encoder(QString *id, QVariantMap *settings, QString *suffix) const
{
    const EncoderAssistant::Encoder enc = profile_model->getSelectedEncoderFromCurrentIndex();
    const Parameters params = profile_model->getSelectedEncoderParametersFromCurrentIndex();

    *suffix = profile_model->getSelectedEncoderSuffixFromCurrentIndex();
    settings->clear();

    // Everything is encoded by the engine itself: the native plugins for
    // FLAC/MP3/Opus, the built-in WAVE writer. Only CUSTOM runs an external
    // command (fed with WAVE on stdin, tagged by Audex afterwards).
    switch (enc) {
    case EncoderAssistant::LAME: {
        *id = u"mp3"_s;
        const int preset = params.value(ENCODER_LAME_PRESET_KEY, ENCODER_LAME_PRESET).toInt();
        const int bitrate = params.value(ENCODER_LAME_BITRATE_KEY, ENCODER_LAME_BITRATE).toInt();
        switch (preset) {
        case ENCODER_LAME_PRESET_MEDIUM:
            (*settings)[u"mode"_s] = u"vbr"_s;
            (*settings)[u"vbrQuality"_s] = 4;
            break;
        case ENCODER_LAME_PRESET_STANDARD:
            (*settings)[u"mode"_s] = u"vbr"_s;
            (*settings)[u"vbrQuality"_s] = 2;
            break;
        case ENCODER_LAME_PRESET_EXTREME:
            (*settings)[u"mode"_s] = u"vbr"_s;
            (*settings)[u"vbrQuality"_s] = 0;
            break;
        case ENCODER_LAME_PRESET_INSANE:
            (*settings)[u"mode"_s] = u"cbr"_s;
            (*settings)[u"bitrate"_s] = u"320"_s;
            break;
        default: // custom (cbr/abr with explicit bitrate)
            (*settings)[u"mode"_s] = u"cbr"_s;
            (*settings)[u"bitrate"_s] = snapLameBitrate(bitrate);
            break;
        }
        return;
    }
    case EncoderAssistant::FLAC:
        *id = u"flac"_s;
        (*settings)[u"compression"_s] = params.value(ENCODER_FLAC_COMPRESSION_KEY, ENCODER_FLAC_COMPRESSION).toInt();
        (*settings)[u"verify"_s] = true;
        return;
    case EncoderAssistant::OPUSENC:
        *id = u"opus"_s;
        (*settings)[u"bitrate"_s] = params.value(ENCODER_OPUSENC_BITRATE_KEY, ENCODER_OPUSENC_BITRATE).toInt();
        (*settings)[u"complexity"_s] = 10;
        return;
    case EncoderAssistant::WAVE:
        *id = u"wav"_s;
        return;
    default:
        break;
    }

    *id = u"external"_s;
    (*settings)[u"commandArgs"_s] = externalCommand(enc, params);
    (*settings)[u"suffix"_s] = *suffix;
}

QStringList
RipRequestBuilder::externalCommand(const EncoderAssistant::Encoder encoder, const Parameters &parameters, QList<Audex::Encoding::CommandIssue> *issues) const
{
    if (issues)
        issues->clear();
    if (encoder != EncoderAssistant::CUSTOM)
        return {};

    const QString scheme = parameters.value(ENCODER_CUSTOM_COMMAND_SCHEME_KEY, ENCODER_CUSTOM_COMMAND_SCHEME).toString();
    const Audex::Encoding::CommandScheme command = Audex::Encoding::parseCommandScheme(scheme, albumVars());
    if (issues)
        *issues = command.issues;
    return command.arguments;
}

// ---- file name schemes --------------------------------------------------------

QString RipRequestBuilder::sanitizePathSegment(const QString &name) const
{
    QString result = name;
    static const QString forbidden = u"/\\:*?\"<>|"_s;
    for (QChar &c : result)
        if (c.unicode() < 0x20 || forbidden.contains(c))
            c = u'_';
    result = result.simplified();
    while (result.startsWith(u'.'))
        result.remove(0, 1);
    if (replaceSpaces())
        result.replace(u' ', u'_');
    return result.trimmed();
}

bool RipRequestBuilder::replaceSpaces() const
{
    return !isImageFile() && columnBool(PROFILE_MODEL_COLUMN_UNDERSCORE_INDEX);
}

// .cdg files only with the detection on (the setting depends on it) and not
// for a disc it found without graphics; a disc it has not checked (yet) is
// checked by the rip first
bool RipRequestBuilder::wantsCdg() const
{
    return Preferences::cdgDetect() && Preferences::cdgRead() && cdda_model->cdInfo().cdg().value_or(true);
}

// The values a scheme can fill in, by placeholder name (utils/schemeparser.h)
QMap<QString, QString> RipRequestBuilder::albumVars() const
{
    const Audex::CDInfo &info = cdda_model->cdInfo();
    const Audex::Metadata::Album &album = info.metadata();
    const int cdNo = album.number(Field::DiscNumber);
    int audioSectors = 0;
    for (const int n : info.audioTrackNumbers())
        if (const auto entry = info.entry(n))
            audioSectors += entry->sectorCount();

    QMap<QString, QString> vars;
    vars[QStringLiteral(VAR_NO_OF_TRACKS)] = QString::number(info.audioTrackNumbers().size());
    vars[QStringLiteral(VAR_ALBUM_ARTIST)] = album.text(Field::Artist);
    vars[QStringLiteral(VAR_ALBUM_TITLE)] = album.text(Field::Album);
    vars[QStringLiteral(VAR_DATE)] = album.text(Field::Year);
    vars[QStringLiteral(VAR_GENRE)] = album.text(Field::Genre);
    vars[QStringLiteral(VAR_CD_NO)] = cdNo > 0 ? QString::number(cdNo) : QString();
    vars[QStringLiteral(VAR_ENCODER)] = profile_model->getSelectedEncoderNameAndVersion().trimmed();
    vars[QStringLiteral(VAR_DISCID)] = info.cddbDiscId();
    vars[QStringLiteral(VAR_MCN)] = album.text(Field::MCN);
    vars[QStringLiteral(VAR_TODAY)] = QDate::currentDate().toString(Qt::ISODate);
    vars[QStringLiteral(VAR_NOW)] = QTime::currentTime().toString(u"hh-mm-ss"_s);
    vars[QStringLiteral(VAR_AUDEX)] = m_application;
    vars[QStringLiteral(VAR_CD_SIZE)] = QString::number(qint64(audioSectors) * Audex::Cdda::SectorBytes / (1024 * 1024)) + u" MiB"_s;
    vars[QStringLiteral(VAR_CD_LENGTH)] = Audex::Cdda::sectorsToTime(audioSectors);
    vars[QStringLiteral(VAR_LINEBREAK)] = u" "_s;
    return vars;
}

bool RipRequestBuilder::fat32() const
{
    return !isImageFile() && columnBool(PROFILE_MODEL_COLUMN_FAT32COMPATIBLE_INDEX);
}

QString RipRequestBuilder::fileNameValue(const QString &value) const
{
    const QString segment = sanitizePathSegment(value);
    return fat32() ? Audex::Scheme::fat32Compatible(segment) : segment;
}

Placeholders RipRequestBuilder::fileNameValues(const QString &suffix) const
{
    Placeholders values;
    const QMap<QString, QString> vars = albumVars();
    for (auto it = vars.cbegin(); it != vars.cend(); ++it)
        values.insert(it.key(), fileNameValue(it.value()));
    values.insert(QStringLiteral(VAR_SUFFIX), fileNameValue(suffix));
    return values;
}

QString RipRequestBuilder::fileName(const QString &scheme, const Placeholders &values, const QString &suffix, QString *error) const
{
    SchemeParser parser;
    const QString parsed = parser.parseScheme(scheme, values);
    if (parser.error()) {
        if (error)
            *error = parser.errorString();
        return QString();
    }

    QStringList segments;
    for (const QString &segment : parsed.split(u'/', Qt::SkipEmptyParts)) {
        const QString s = segment.trimmed();
        if (!s.isEmpty() && s != u"."_s && s != u".."_s)
            segments << s;
    }
    if (segments.isEmpty()) {
        if (error)
            *error = i18n("The name scheme results in an empty file name.");
        return QString();
    }

    const QString name = segments.join(u'/');
    // a scheme may spell the extension out instead of using $suffix
    const bool hasSuffix = name.endsWith(u'.' + suffix, Qt::CaseInsensitive) || (suffix == u"jpg"_s && name.endsWith(u".jpeg"_s, Qt::CaseInsensitive));
    return hasSuffix ? name : name + u'.' + suffix;
}

QString RipRequestBuilder::resolveNameScheme(const QString &scheme, const QString &suffix) const
{
    const QString name = fileName(scheme, fileNameValues(suffix), suffix);
    return name.isEmpty() ? u"audex."_s + suffix : name; // validate() reports the scheme
}

QString RipRequestBuilder::logFilePath() const
{
    if (!columnBool(PROFILE_MODEL_COLUMN_LOG_INDEX))
        return {};
    return QDir(outputDirectory()).filePath(resolveNameScheme(column(PROFILE_MODEL_COLUMN_LOG_NAME_INDEX), u"log"_s));
}

// ---- request ------------------------------------------------------------------

bool RipRequestBuilder::isImageFile() const
{
    return profile_model->isCurrentImage();
}

QString RipRequestBuilder::outputDirectory() const
{
    const QMap<int, QString> paths = filePaths();
    if (paths.isEmpty())
        return QDir(Preferences::basePath()).path();
    return QFileInfo(paths.first()).absolutePath();
}

QMap<int, QString> RipRequestBuilder::filePaths(QString *error) const
{
    const Audex::CDInfo &info = cdda_model->cdInfo();
    const Audex::Metadata::Album &album = info.metadata();

    QString id, suffix;
    QVariantMap settings;
    encoder(&id, &settings, &suffix);
    if (const Audex::Encoding::EncoderFactory *factory = m_encoders ? m_encoders->factory(id) : nullptr)
        suffix = factory->fileSuffix(settings); // what the engine writes

    const QDir base(Preferences::basePath());
    QMap<int, QString> result;
    const QList<int> tracks = cdda_model->selectedTracks();
    Placeholders values = fileNameValues(suffix);

    if (isImageFile()) {
        QString scheme = column(PROFILE_MODEL_COLUMN_IMAGE_SCHEME_INDEX);
        if (scheme.trimmed().isEmpty())
            scheme = QStringLiteral(DEFAULT_IMAGE_SCHEME);
        const QString name = fileName(scheme, values, suffix, error);
        if (name.isEmpty())
            return {};
        for (const int number : tracks)
            result.insert(number, base.filePath(name));
        return result;
    }

    QString scheme = column(PROFILE_MODEL_COLUMN_SCHEME_INDEX);
    if (scheme.trimmed().isEmpty())
        scheme = QStringLiteral(DEFAULT_SCHEME);
    const bool twoDigits = columnBool(PROFILE_MODEL_COLUMN_2DIGITSTRACKNUM_INDEX);

    for (const int number : tracks) {
        const Audex::Metadata::Track &track = album.track(number);
        QString trackArtist = track.text(Field::Artist);
        if (trackArtist.isEmpty())
            trackArtist = album.text(Field::Artist);
        QString trackTitle = track.text(Field::Title);
        if (trackTitle.isEmpty())
            trackTitle = number == 0 ? i18n("Hidden Track") : i18n("Track %1", number);
        const int trackNo = info.displayTrackNumber(number);

        values.insert(QStringLiteral(VAR_TRACK_ARTIST), fileNameValue(trackArtist));
        values.insert(QStringLiteral(VAR_TRACK_TITLE), fileNameValue(trackTitle));
        values.insert(QStringLiteral(VAR_TRACK_NO), twoDigits ? u"%1"_s.arg(trackNo, 2, 10, QLatin1Char('0')) : QString::number(trackNo));
        values.insert(QStringLiteral(VAR_ISRC), fileNameValue(track.text(Field::ISRC)));

        const QString name = fileName(scheme, values, suffix, error);
        if (name.isEmpty())
            return {};
        result.insert(number, base.filePath(name));
    }
    return result;
}

bool RipRequestBuilder::validate(QString *error, QStringList *existingFiles) const
{
    if (cdda_model->isEmpty()) {
        *error = i18n("No disc in drive.");
        return false;
    }
    const QList<int> tracks = cdda_model->selectedTracks();
    if (tracks.isEmpty()) {
        *error = i18n("No tracks selected.");
        return false;
    }

    QString id, suffix;
    QVariantMap settings;
    encoder(&id, &settings, &suffix);
    if (!m_encoders || !m_encoders->factory(id)) {
        *error = i18n("The output format \"%1\" is not available.", id);
        return false;
    }
    if (id == u"external"_s) {
        QList<Audex::Encoding::CommandIssue> issues;
        externalCommand(profile_model->getSelectedEncoderFromCurrentIndex(), profile_model->getSelectedEncoderParametersFromCurrentIndex(), &issues);
        if (!issues.isEmpty()) {
            QStringList messages;
            for (const Audex::Encoding::CommandIssue &issue : issues)
                messages << SchemeParser::commandIssueText(issue);
            *error = i18n("The encoder command cannot be used. Please edit the command in the profile.") + u'\n' + messages.join(u'\n');
            return false;
        }
        if (settings.value(u"commandArgs"_s).toStringList().isEmpty()) {
            *error = i18n("No encoder command configured.");
            return false;
        }
        if (suffix.trimmed().isEmpty()) {
            *error = i18n("Please set a file suffix for the custom encoder in the profile.");
            return false;
        }
    }

    QString schemeError;
    const QMap<int, QString> paths = filePaths(&schemeError);
    if (paths.isEmpty()) {
        *error = i18n("The file name scheme is invalid: %1", schemeError);
        return false;
    }

    // the other files the profile writes
    struct Extra {
        int enabled;
        int scheme;
        QString suffix;
    };
    const QList<Extra> extras{{PROFILE_MODEL_COLUMN_SC_INDEX, PROFILE_MODEL_COLUMN_SC_NAME_INDEX, u"jpg"_s},
                              {PROFILE_MODEL_COLUMN_LOG_INDEX, PROFILE_MODEL_COLUMN_LOG_NAME_INDEX, u"log"_s},
                              isImageFile() ? Extra{PROFILE_MODEL_COLUMN_CUE_INDEX, PROFILE_MODEL_COLUMN_CUE_NAME_INDEX, u"cue"_s}
                                            : Extra{PROFILE_MODEL_COLUMN_PL_INDEX, PROFILE_MODEL_COLUMN_PL_NAME_INDEX, u"m3u"_s}};
    for (const Extra &extra : extras) {
        if (columnBool(extra.enabled) && fileName(column(extra.scheme), fileNameValues(extra.suffix), extra.suffix, &schemeError).isEmpty()) {
            *error = i18n("The name scheme \"%1\" is invalid: %2", column(extra.scheme), schemeError);
            return false;
        }
    }

    if (!isImageFile() && QSet<QString>(paths.cbegin(), paths.cend()).size() != paths.size()) {
        *error = i18n("Several tracks would be written to the same file. Use a file name scheme that contains the track number.");
        return false;
    }

    const QString outDir = QFileInfo(paths.first()).absolutePath();
    if (!QDir().mkpath(outDir)) {
        *error = i18n("Cannot create the output folder %1.", outDir);
        return false;
    }

    // mkpath() also succeeds for an existing folder Audex cannot write to
    QTemporaryFile probe(QDir(outDir).filePath(u".audex-XXXXXX"_s));
    if (!probe.open()) {
        *error = i18n("Cannot write to the output folder %1: %2", outDir, probe.errorString());
        return false;
    }

    if (existingFiles && !Preferences::overwriteExistingFiles()) {
        QStringList candidates(paths.cbegin(), paths.cend());
        const PostProcessPlan p = plan(); // cover, playlist and cue sheet are written as well
        candidates << p.coverFilePath << p.playlistFilePath << p.cueFilePath << logFilePath();
        if (wantsCdg()) // .cdg files next to the audio files
            for (const QString &path : paths)
                candidates << QDir(QFileInfo(path).path()).filePath(QFileInfo(path).completeBaseName() + u".cdg"_s);
        for (const QString &path : std::as_const(candidates))
            if (!path.isEmpty() && !existingFiles->contains(path) && QFile::exists(path))
                existingFiles->append(path);
    }

    return true;
}

RipRequestBuilder::StreamWarning RipRequestBuilder::streamWarning() const
{
    StreamWarning w;
    const Audex::Rip::DriveFeatures f = DeviceSettings::features(m_driveUdi);
    if (f.accurateStream == Audex::Rip::Feature::No) {
        w.warn = true;
        w.measured = true;
        w.jitterSamples = f.jitterSamples;
    } else if (f.accurateStream == Audex::Rip::Feature::Unknown) {
        const std::optional<bool> reported = Audex::reportedAccurateStream(m_drive.id);
        w.warn = reported.has_value() && !*reported;
    }
    return w;
}

RipRequestBuilder::DiskSpace RipRequestBuilder::diskSpace() const
{
    DiskSpace space;
    space.folder = outputDirectory();

    qint64 audio = 0;
    const Audex::CDInfo &info = cdda_model->cdInfo();
    for (const int number : cdda_model->selectedTracks()) {
        const auto entry = info.entry(number);
        if (entry && entry->audio)
            audio += qint64(entry->sectorCount()) * Audex::Cdda::SectorBytes;
    }
    // even 320 kbit/s are less than a quarter of CD audio
    const EncoderAssistant::Encoder encoder = profile_model->getSelectedEncoderFromCurrentIndex();
    const bool lossy = encoder == EncoderAssistant::LAME || encoder == EncoderAssistant::OPUSENC;
    qint64 needed = lossy ? audio / 3 : audio;
    // a repaired image keeps the original next to it
    if (isImageFile() && columnBool(PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX) && columnBool(PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX))
        needed *= 2;
    space.needed = needed + 16 * 1024 * 1024; // cover, cue sheet, log, file system

    const QStorageInfo storage(space.folder);
    if (storage.isValid() && storage.isReady())
        space.available = storage.bytesAvailable();
    return space;
}

Audex::RipRequest RipRequestBuilder::request() const
{
    Audex::RipRequest rq;
    rq.drive = m_drive;
    rq.disc = cdda_model->cdInfo();
    rq.tracks = cdda_model->selectedTracks();

    // read options are per drive (Device settings page)
    const DeviceSettings::Values device = DeviceSettings::load(m_driveUdi);
    rq.options.mode = device.secureMode ? Audex::Rip::ReadMode::Secure : Audex::Rip::ReadMode::Fast;
    rq.options.readOffset = device.sampleShift;
    rq.options.readErrorRetries = device.retriesOnReadError;
    rq.options.overread = device.overread;
    rq.options.useC2 = device.useC2;
    rq.options.cacheDefeat = device.cacheDefeat;
    rq.options.readSpeed = device.readSpeed;
    rq.options.errorReadSpeed = device.errorReadSpeed;
    rq.options.cacheDefeatReads = DeviceSettings::cacheDefeatReads(m_driveUdi); // 0: RipJob measures
    rq.options.burstSectors = DeviceSettings::features(m_driveUdi).burstSectors; // 0: whatever the drive allows
    if (m_drive.isSimulated())
        rq.options.cacheDefeatDistance = 1500; // the demo disc is short

    rq.accurateRipLookup = Preferences::accurateRipVerify();
    rq.ctdbRepair = isImageFile() && columnBool(PROFILE_MODEL_COLUMN_CTDB_REPAIR_INDEX);
    rq.ctdbRepairKeepOriginal = columnBool(PROFILE_MODEL_COLUMN_CTDB_REPAIR_KEEP_ORIGINAL_INDEX);
    rq.ctdbLookup = Preferences::ctdbVerify() || rq.ctdbRepair;
    // Secure mode keeps a track that a database confirms after one read: a
    // second pass cannot add anything to that. In an image, the tracks read
    // again replace their first read after the rip (see RipJob).
    rq.keepConfirmedTracks = (rq.accurateRipLookup || rq.ctdbLookup) && device.secureMode;
    // ISRCs and MCN are collected by the same Q sub-channel scan
    // ISRCs and MCN are collected by the same Q sub-channel scan. A drive the
    // drive test found without usable Q data is not asked: it may only time out.
    const bool noSubchannel = DeviceSettings::features(m_driveUdi).subchannelQ == Audex::Rip::Feature::No;
    rq.scanSubchannel = Preferences::detectGaps() && !noSubchannel;
    rq.subchannelSkipped = Preferences::detectGaps() && noSubchannel;
    rq.scanIsrcMcn = rq.scanSubchannel && Preferences::readIsrcMcn();
    // an image always contains the whole disc; for track files the pre-gaps
    // are known from the Q sub-channel scan only
    rq.pregapsWithTrack = !isImageFile() && Preferences::detectGaps() && Preferences::gapHandling() == Preferences::EnumGapHandling::WithOwnTrack;
    // CD+G needs the raw sub-channel
    const bool noRawSubchannel = DeviceSettings::features(m_driveUdi).rwSubchannel == Audex::Rip::Feature::No;
    rq.readCdg = wantsCdg() && !noRawSubchannel;
    rq.cdgSkipped = wantsCdg() && noRawSubchannel;
    rq.cdgDetected = cdda_model->cdInfo().cdg().value_or(false);

    const StreamWarning stream = streamWarning();
    rq.inaccurateStream = stream.warn;
    rq.inaccurateStreamMeasured = stream.measured;
    rq.jitterSamples = stream.jitterSamples;

    rq.outputDirectory = outputDirectory();
    rq.logFilePath = logFilePath();
    rq.filePaths = filePaths();
    rq.imageFile = isImageFile();

    rq.encoders = m_encoders;
    QString suffix;
    encoder(&rq.encoderId, &rq.encoderSettings, &suffix);

    rq.writeTags = true;
    rq.preEmphasisTag = Preferences::preEmphasisTag().trimmed();
    rq.detectHdcd = Preferences::hdcdDetect();
    rq.hdcdTag = rq.detectHdcd ? Preferences::hdcdTag().trimmed() : QString();
    rq.embedCover = profile_model->isSelectedEncoderWithEmbedCover();
    const QSize size = columnVariant(PROFILE_MODEL_COLUMN_SC_SIZE_INDEX).toSize();
    rq.coverMaxSize = columnBool(PROFILE_MODEL_COLUMN_SC_SCALE_INDEX) ? qMax(size.width(), size.height()) : 0;

    rq.application = m_application;
    return rq;
}

PostProcessPlan RipRequestBuilder::plan() const
{
    PostProcessPlan p;
    const QDir outDir(outputDirectory());
    p.imageFile = isImageFile();

    p.saveCover = columnBool(PROFILE_MODEL_COLUMN_SC_INDEX) && !cdda_model->cover().isNull();
    if (p.saveCover) {
        p.coverScale = columnBool(PROFILE_MODEL_COLUMN_SC_SCALE_INDEX);
        p.coverSize = columnVariant(PROFILE_MODEL_COLUMN_SC_SIZE_INDEX).toSize();
        p.coverFormat = column(PROFILE_MODEL_COLUMN_SC_FORMAT_INDEX).toUpper();
        if (p.coverFormat != u"PNG"_s)
            p.coverFormat = u"JPEG"_s;
        p.coverFilePath = outDir.filePath(resolveNameScheme(column(PROFILE_MODEL_COLUMN_SC_NAME_INDEX), p.coverFormat == u"PNG"_s ? u"png"_s : u"jpg"_s));
    }

    p.writePlaylist = columnBool(PROFILE_MODEL_COLUMN_PL_INDEX) && !p.imageFile;
    if (p.writePlaylist) {
        p.playlistAbsolutePaths = columnBool(PROFILE_MODEL_COLUMN_PL_ABS_FILE_PATH_INDEX);
        p.playlistUtf8 = columnBool(PROFILE_MODEL_COLUMN_PL_UTF8_INDEX);
        p.playlistFilePath = outDir.filePath(resolveNameScheme(column(PROFILE_MODEL_COLUMN_PL_NAME_INDEX), u"m3u"_s));
    }

    // cue sheets are tied to the disc image (image file rip)
    p.writeCue = isImageFile() && columnBool(PROFILE_MODEL_COLUMN_CUE_INDEX);
    if (p.writeCue) {
        p.cueMcnIsrc = columnBool(PROFILE_MODEL_COLUMN_CUE_MCN_ISRC_INDEX);
        p.cueFilePath = outDir.filePath(resolveNameScheme(column(PROFILE_MODEL_COLUMN_CUE_NAME_INDEX), u"cue"_s));
    }

    p.runHook = columnBool(PROFILE_MODEL_COLUMN_HOOK_INDEX);
    if (p.runHook) {
        p.hookCommand = column(PROFILE_MODEL_COLUMN_HOOK_COMMAND_INDEX);
        p.hookVars = albumVars();
    }
    p.outputDir = outDir.absolutePath();

    return p;
}

// ---- post processing ------------------------------------------------------------

void RipRequestBuilder::runPostProcess(const PostProcessPlan &plan,
                                       const Audex::RipSummary &summary,
                                       const Audex::CDInfo &disc,
                                       const QList<int> &tracks,
                                       QObject *parent,
                                       const std::function<void(int, const QString &)> &log)
{
    Q_UNUSED(parent);

    if (!summary.completed)
        return;

    auto writeTextFile = [&log](const QString &path, const QStringList &lines) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            log(int(Audex::Rip::LogLevel::Warning), i18n("Cannot write %1: %2", path, file.errorString()));
            return false;
        }
        file.write(lines.join(u'\n').toUtf8() + '\n');
        return true;
    };

    // cover file
    if (plan.saveCover) {
        QImage image = QImage::fromData(disc.metadata().cover().data);
        if (image.isNull()) {
            log(int(Audex::Rip::LogLevel::Warning), i18n("The cover cannot be decoded and is not saved."));
        } else {
            if (plan.coverScale && plan.coverSize.isValid())
                image = image.scaled(plan.coverSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            if (image.save(plan.coverFilePath, plan.coverFormat.toLatin1().constData()))
                log(int(Audex::Rip::LogLevel::Info), i18n("Cover saved to %1.", plan.coverFilePath));
            else
                log(int(Audex::Rip::LogLevel::Warning), i18n("Cannot save the cover to %1.", plan.coverFilePath));
        }
    }

    // playlist
    if (plan.writePlaylist && !summary.files.isEmpty()) {
        Playlist playlist;
        const QDir playlistDir = QFileInfo(plan.playlistFilePath).absoluteDir();
        for (int i = 0; i < summary.files.count(); ++i) {
            const QString &file = summary.files.at(i);
            PlaylistItem item;
            item.setFilename(plan.playlistAbsolutePaths ? file : playlistDir.relativeFilePath(file));
            const int number = plan.imageFile ? -1 : tracks.value(i, -1);
            const Audex::Metadata::Track &track = disc.metadata().track(number);
            QString artist = track.text(Field::Artist);
            if (artist.isEmpty())
                artist = disc.metadata().text(Field::Artist);
            item.setArtist(artist);
            item.setTitle(plan.imageFile ? disc.metadata().text(Field::Album) : track.text(Field::Title));
            int sectors = 0;
            if (plan.imageFile) {
                for (const int n : tracks)
                    if (const auto entry = disc.entry(n))
                        sectors += entry->sectorCount();
            } else if (const auto entry = disc.entry(number)) {
                sectors = entry->sectorCount();
            }
            item.setLength(sectors / Audex::Cdda::SectorsPerSecond);
            playlist.appendItem(item);
        }
        const QByteArray data = playlist.toM3U(playlistDir.path(), plan.playlistUtf8);
        QFile file(plan.playlistFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            log(int(Audex::Rip::LogLevel::Info), i18n("Playlist saved to %1.", plan.playlistFilePath));
        } else {
            log(int(Audex::Rip::LogLevel::Warning), i18n("Cannot write %1: %2", plan.playlistFilePath, file.errorString()));
        }
    }

    // cue sheet
    if (plan.writeCue && !summary.files.isEmpty()) {
        CueSheetWriter writer(disc);
        writer.setSubchannel(summary.subchannel);
        const QStringList cue = plan.imageFile ? writer.cueSheet(summary.files.first(), tracks, plan.cueMcnIsrc, plan.cueMcnIsrc)
                                               : writer.cueSheet(summary.files, tracks, plan.cueMcnIsrc, plan.cueMcnIsrc);
        writeTextFile(plan.cueFilePath, cue);
    }

    // user command hook: runs as a child process, its completion is logged.
    // The process belongs to the application, so it keeps running when the
    // progress dialog is closed (logging stops then); quitting Audex ends it.
    if (plan.runHook && !plan.hookCommand.isEmpty()) {
        const QStringList args = Audex::Encoding::hookCommandArguments(plan.hookCommand, plan.hookVars, summary.files, plan.outputDir);
        const QString display = Audex::Encoding::commandToString(args);
        if (args.isEmpty()) {
            log(int(Audex::Rip::LogLevel::Warning), i18n("Cannot start command: %1", plan.hookCommand));
        } else {
            const QPointer<QObject> logTo(parent);
            auto *process = new QProcess(QCoreApplication::instance());
            process->setStandardOutputFile(QProcess::nullDevice());
            QObject::connect(process, &QProcess::started, process, [logTo, log, display] {
                if (logTo)
                    log(int(Audex::Rip::LogLevel::Info), i18n("Command started: %1", display));
            });
            QObject::connect(process, &QProcess::errorOccurred, process, [process, logTo, log, display](QProcess::ProcessError error) {
                if (error != QProcess::FailedToStart)
                    return;
                if (logTo)
                    log(int(Audex::Rip::LogLevel::Warning), i18n("Cannot start command: %1", display));
                process->deleteLater();
            });
            QObject::connect(process, &QProcess::finished, process, [process, logTo, log, display](int exitCode, QProcess::ExitStatus status) {
                if (logTo) {
                    const QString err = QString::fromLocal8Bit(process->readAllStandardError()).right(16384).trimmed();
                    if (status == QProcess::NormalExit && exitCode == 0)
                        log(int(Audex::Rip::LogLevel::Info), i18n("Command finished: %1", display));
                    else if (err.isEmpty())
                        log(int(Audex::Rip::LogLevel::Warning), i18n("The command failed (exit code %1): %2", exitCode, display));
                    else
                        log(int(Audex::Rip::LogLevel::Warning), i18n("The command failed (exit code %1): %2 — %3", exitCode, display, err));
                }
                process->deleteLater();
            });
            process->start(args.constFirst(), args.sliced(1));
        }
    }
}
