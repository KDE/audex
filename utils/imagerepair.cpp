/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "imagerepair.h"

#include "core/checksums.h"
#include "core/ctdbparity.h"
#include "encoding/encodedoutputs.h"
#include "utils/fileops.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

using namespace Qt::StringLiterals;

namespace Audex
{

namespace
{

constexpr int WordsPerSector = Cdda::SectorBytes / 2;

struct Tr {
    Q_DECLARE_TR_FUNCTIONS(Audex::ImageRepair)
};

QString hex8(quint32 v)
{
    return u"%1"_s.arg(v, 8, 16, QLatin1Char('0'));
}

// Reads the whole file and hands it to consume() in parts; frames is known
// before the first part
bool decodeFile(const Encoding::EncoderFactory &factory,
                const QString &path,
                const std::function<bool(QByteArrayView)> &consume,
                const std::function<bool()> &isCanceled,
                Text *error,
                qint64 *frames = nullptr)
{
    const std::unique_ptr<Encoding::AudioDecoder> decoder = factory.createDecoder();
    if (!decoder) {
        *error = Text::translate("Audex::ImageRepair", "%1 files cannot be read back.").arg(factory.displayName());
        return false;
    }
    if (!decoder->open(path, error))
        return false;
    if (frames)
        *frames = decoder->totalFrames();
    QByteArray pcm;
    while (true) {
        if (isCanceled && isCanceled()) {
            *error = Text::translate("Audex::ImageRepair", "canceled");
            return false;
        }
        if (!decoder->read(pcm, error))
            return false;
        if (pcm.isEmpty())
            return true;
        if (!consume(pcm))
            return false;
    }
}

// "mm:ss.ff" ranges of the corrected sectors, neighbours within 5 sectors merged
QString positions(const QList<Ctdb::Correction> &corrections, int firstLba)
{
    QStringList ranges;
    int from = -1;
    int to = -1;
    const auto flush = [&] {
        if (from < 0)
            return;
        ranges << (from == to ? Cdda::sectorsToMsf(from) : u"%1-%2"_s.arg(Cdda::sectorsToMsf(from), Cdda::sectorsToMsf(to)));
    };
    for (const Ctdb::Correction &c : corrections) {
        const int lba = firstLba + int(c.word / WordsPerSector);
        if (from >= 0 && lba - to <= 5) {
            to = lba;
            continue;
        }
        flush();
        from = to = lba;
    }
    flush();
    if (ranges.size() > 20) {
        const qsizetype more = ranges.size() - 20;
        ranges = ranges.mid(0, 20);
        ranges << u"and %1 more"_s.arg(more);
    }
    return ranges.join(u", "_s);
}

// An image file as a drive: sectors from its decoded audio, read one after
// the other (the engine reads a finished image in fast mode sequentially).
// Keeps the last part decoded, for reads that start a little further back.
class ImageSectorReader : public Rip::SectorReader
{
public:
    ImageSectorReader(Encoding::AudioDecoder &decoder, int firstLba)
        : m_decoder(decoder)
        , m_firstLba(firstLba)
    {
    }

    Rip::SectorReadResult read(int lba, int count) override
    {
        Rip::SectorReadResult r;
        r.audio = QByteArray(qsizetype(count) * Cdda::SectorBytes, '\0');
        r.sectors.resize(count);
        const qint64 begin = qint64(lba - m_firstLba) * Cdda::SectorBytes;
        const qint64 end = begin + r.audio.size();
        while (!m_end && m_start + m_buffer.size() < end) {
            QByteArray pcm;
            if (!m_decoder.read(pcm, &m_error) || pcm.isEmpty())
                m_end = true;
            m_buffer.append(pcm);
        }
        for (int i = 0; i < count; ++i) {
            const qint64 at = begin + qint64(i) * Cdda::SectorBytes;
            if (at < m_start || at + Cdda::SectorBytes > m_start + m_buffer.size())
                continue; // not ok: outside of what is decoded
            std::memcpy(r.audio.data() + qsizetype(i) * Cdda::SectorBytes, m_buffer.constData() + (at - m_start), Cdda::SectorBytes);
            r.sectors[i].ok = true;
        }
        if (!r.allOk())
            r.error = m_error.isEmpty() ? u"beyond the decoded audio"_s : m_error.toEnglish();
        // keep a margin behind this read
        const qint64 keepFrom = begin - qint64(KeepSectors) * Cdda::SectorBytes;
        if (keepFrom > m_start) {
            m_buffer.remove(0, qsizetype(std::min<qint64>(keepFrom - m_start, m_buffer.size())));
            m_start = keepFrom;
        }
        return r;
    }

    int maxSectorsPerRead() const override
    {
        return 64;
    }

    QString description() const override
    {
        return u"image file"_s;
    }

    Text error() const
    {
        return m_error;
    }

private:
    static constexpr int KeepSectors = 1000;
    Encoding::AudioDecoder &m_decoder;
    int m_firstLba = 0;
    QByteArray m_buffer;
    qint64 m_start = 0; // image byte of m_buffer[0]
    bool m_end = false;
    Text m_error;
};

}

QString unrepairedPath(const QString &path)
{
    const QFileInfo info(path);
    return QDir(info.path()).filePath(info.completeBaseName() + u".unrepaired."_s + info.suffix());
}

ImageRepairResult repairImage(const ImageRepairRequest &rq, const std::function<void(const QString &)> &message, const std::function<bool()> &isCanceled)
{
    ImageRepairResult result;
    result.log << Text::translate("Audex::ImageRepair", "CUETools database (CTDB) repair") << Text();
    const auto note = [&](const Text &line) {
        result.log << Text::plain(u"     %1"_s).arg(line);
    };
    const auto say = [&](const QString &text) {
        if (message)
            message(text);
    };

    const Cdda::Track *first = rq.toc.track(rq.toc.firstAudioTrackNumber());
    const Cdda::Track *last = rq.toc.track(rq.toc.lastAudioTrackNumber());
    if (!rq.encoder || !first || !last || first->firstLba < rq.firstLba) {
        note(Text::translate("Audex::ImageRepair", "Not possible for this image."));
        return result;
    }
    const qint64 offsetWords = qint64(first->firstLba - rq.firstLba) * WordsPerSector;
    Ctdb::Parity parity(offsetWords, qint64(last->lastLba + 1 - first->firstLba) * WordsPerSector);
    if (!parity.isValid()) {
        note(Text::translate("Audex::ImageRepair", "Not possible: the disc is too short or too long."));
        return result;
    }

    QList<Ctdb::Entry> candidates;
    for (const Ctdb::Entry &e : rq.entries)
        if (Ctdb::isSameAudio(e, rq.toc) && !e.syndromes.isEmpty())
            candidates.append(e);
    std::stable_sort(candidates.begin(), candidates.end(), [](const Ctdb::Entry &a, const Ctdb::Entry &b) {
        return a.confidence > b.confidence;
    });
    if (candidates.isEmpty()) {
        note(Text::translate("Audex::ImageRepair", "The database has no recovery data for this disc."));
        return result;
    }

    // --- read the image back ---
    say(Tr::tr("Reading the image for the repair..."));
    Text error;
    if (!decodeFile(
            *rq.encoder,
            rq.path,
            [&](QByteArrayView pcm) {
                parity.update(pcm);
                return true;
            },
            isCanceled,
            &error)
        || !parity.isComplete()) {
        note(Text::translate("Audex::ImageRepair", "Reading %1 failed: %2")
                 .arg(QFileInfo(rq.path).fileName())
                 .arg(error.isEmpty() ? Text::translate("Audex::ImageRepair", "the image is too short") : error));
        return result;
    }

    // --- an entry the image can be restored to ---
    const auto fetch = rq.fetchParity ? rq.fetchParity : [&isCanceled](const Ctdb::Entry &e, Text *err) {
        return Ctdb::fetchParity(e, err, 60000, isCanceled);
    };
    const Ctdb::Entry *chosen = nullptr;
    int shift = 0;
    QList<Ctdb::Correction> corrections;
    QList<Text> skipped;
    for (const Ctdb::Entry &e : std::as_const(candidates)) {
        const Text name = Text::translate("Audex::ImageRepair", "[%1] (confidence %2)").arg(hex8(e.crc)).arg(e.confidence);
        const std::optional<Ctdb::ShiftMatch> match = parity.findShift(e.syndromes, e.crc);
        if (!match) {
            skipped << Text::translate("Audex::ImageRepair", "%1: too different").arg(name);
            continue;
        }
        if (!match->errors) {
            note(Text::translate("Audex::ImageRepair", "The image matches %1, nothing to repair.").arg(name));
            return result;
        }
        if (!e.hasParity()) {
            skipped << Text::translate("Audex::ImageRepair", "%1: no recovery data").arg(name);
            continue;
        }
        say(Tr::tr("Downloading the recovery data of the CUETools database..."));
        Text fetchError;
        const QByteArray data = fetch(e, &fetchError);
        if (data.size() < e.syndromes.size() * Ctdb::Stride * 2) {
            skipped << Text::translate("Audex::ImageRepair", "%1: download failed (%2)").arg(name).arg(fetchError);
            continue;
        }
        bool consistent = true; // the recovery data belongs to the entry
        for (int i = 0; i < e.syndromes.size(); ++i)
            consistent = consistent && qFromLittleEndian<quint16>(data.constData() + qsizetype(i) * Ctdb::Stride * 2) == e.syndromes.at(i);
        if (!consistent) {
            skipped << Text::translate("Audex::ImageRepair", "%1: recovery data does not fit the entry").arg(name);
            continue;
        }
        const auto fix = parity.corrections(data, int(e.syndromes.size()), match->shift, e.crc);
        if (!fix) {
            skipped << Text::translate("Audex::ImageRepair", "%1: too much damage").arg(name);
            continue;
        }
        chosen = &e;
        shift = match->shift;
        corrections = *fix;
        break;
    }
    for (const Text &line : std::as_const(skipped))
        note(line);
    if (!chosen) {
        note(Text::translate("Audex::ImageRepair", "The image could not be repaired."));
        return result;
    }
    if (shift == 0)
        note(Text::translate("Audex::ImageRepair", "Entry [%1] (confidence %2)").arg(hex8(chosen->crc)).arg(chosen->confidence));
    else
        note(Text::translate("Audex::ImageRepair", "Entry [%1] (confidence %2), other pressing with offset %3")
                 .arg(hex8(chosen->crc))
                 .arg(chosen->confidence)
                 .arg((shift > 0 ? u"+"_s : QString()) + QString::number(shift)));
    note(Text::translate("Audex::ImageRepair", "Corrections: %1 values at %2").arg(corrections.size()).arg(positions(corrections, rq.firstLba)));

    // --- write the repaired image ---
    say(Tr::tr("Writing the repaired image..."));
    const QString part = Encoding::EncodedOutputs::partialPath(rq.path);
    const std::unique_ptr<Encoding::AudioEncoder> encoder = rq.encoder->create(rq.encoderSettings);
    const qint64 windowBegin = (offsetWords + Ctdb::Stride + 2 * qint64(shift)) * 2; // bytes
    const qint64 windowEnd = windowBegin + qint64(parity.rows()) * Ctdb::Stride * 2;
    Rip::Crc32 crc;
    qint64 position = 0; // bytes
    qsizetype next = 0; // correction
    qint64 frames = 0;
    bool opened = false;
    bool ok = decodeFile(
                  *rq.encoder,
                  rq.path,
                  [&](QByteArrayView view) {
                      if (!opened && !(opened = encoder->open(part, frames, &error)))
                          return false;
                      QByteArray pcm = view.toByteArray();
                      const qint64 end = position + pcm.size();
                      for (; next < corrections.size() && corrections.at(next).word * 2 < end; ++next) {
                          char *p = pcm.data() + (corrections.at(next).word * 2 - position);
                          qToLittleEndian<quint16>(qFromLittleEndian<quint16>(p) ^ corrections.at(next).mask, p);
                      }
                      const qint64 from = std::max(windowBegin, position);
                      const qint64 to = std::min(windowEnd, end);
                      if (from < to)
                          crc.update(QByteArrayView(pcm).sliced(from - position, to - from));
                      position = end;
                      return encoder->write(pcm, &error);
                  },
                  isCanceled,
                  &error,
                  &frames)
        && opened && encoder->finish(&error);
    if (ok && crc.value() != chosen->crc) {
        ok = false;
        error = Text::translate("Audex::ImageRepair", "the repaired audio does not match the entry");
    }
    if (!ok) {
        encoder->abort();
        QFile::remove(part);
        note(Text::translate("Audex::ImageRepair", "Writing the repaired image failed: %1").arg(error));
        return result;
    }

    const QString suffix = QFileInfo(rq.path).suffix();
    if (rq.writeTags && !Encoding::TagWriter::write(part, suffix, rq.tags, &error))
        note(Text::translate("Audex::ImageRepair", "Tagging the repaired image failed: %1").arg(error));

    QString backup;
    if (rq.keepOriginal) {
        const QFileInfo info(unrepairedPath(rq.path));
        backup = info.filePath();
        for (int number = 1; QFileInfo::exists(backup) || QFileInfo(backup).isSymLink(); ++number)
            backup = QDir(info.path()).filePath(info.completeBaseName() + u".%1."_s.arg(number) + info.suffix());
        if (!backupFile(rq.path, backup)) {
            note(Text::translate("Audex::ImageRepair", "Backing up %1 failed; the repaired image is %2")
                     .arg(QFileInfo(rq.path).fileName(), QFileInfo(part).fileName()));
            return result;
        }
    }
    if (!replaceFile(part, rq.path)) {
        if (!backup.isEmpty())
            QFile::remove(backup);
        note(Text::translate("Audex::ImageRepair", "Replacing %1 failed; the repaired image is %2")
                 .arg(QFileInfo(rq.path).fileName(), QFileInfo(part).fileName()));
        return result;
    }

    result.originalPath = backup;
    result.repaired = true;
    result.corrections = int(corrections.size());
    note(Text::translate("Audex::ImageRepair", "Repaired: the image matches the database entry now."));
    if (rq.keepOriginal)
        note(Text::translate("Audex::ImageRepair", "The unrepaired image was kept as %1").arg(QFileInfo(result.originalPath).fileName()));
    return result;
}

bool patchImage(const ImagePatchRequest &rq, Text *error, const std::function<bool()> &isCanceled)
{
    if (!rq.encoder) {
        *error = Text::plain(u"no encoder"_s);
        return false;
    }

    struct Source {
        qint64 begin = 0; // bytes in the image
        qint64 end = 0;
        std::unique_ptr<QFile> file;
    };
    std::vector<Source> sources;
    for (const ImagePatch &patch : rq.patches) {
        auto file = std::make_unique<QFile>(patch.pcmPath);
        if (!file->open(QIODevice::ReadOnly)) {
            *error = Text::plain(u"%1: %2"_s.arg(patch.pcmPath, file->errorString()));
            return false;
        }
        const qint64 size = file->size();
        sources.push_back(Source{patch.offsetBytes, patch.offsetBytes + size, std::move(file)});
    }

    const QString part = Encoding::EncodedOutputs::partialPath(rq.path);
    const std::unique_ptr<Encoding::AudioEncoder> encoder = rq.encoder->create(rq.encoderSettings);
    qint64 position = 0; // bytes of the image read so far
    qint64 frames = 0;
    bool opened = false;
    bool ok = decodeFile(
                  *rq.encoder,
                  rq.path,
                  [&](QByteArrayView view) {
                      if (!opened && !(opened = encoder->open(part, frames, error)))
                          return false;
                      QByteArray pcm = view.toByteArray();
                      const qint64 end = position + pcm.size();
                      for (Source &source : sources) {
                          const qint64 from = std::max(source.begin, position);
                          const qint64 to = std::min(source.end, end);
                          if (from >= to)
                              continue;
                          if (!source.file->seek(from - source.begin) || source.file->read(pcm.data() + (from - position), to - from) != to - from) {
                              *error = Text::plain(u"%1: %2"_s.arg(source.file->fileName(), source.file->errorString()));
                              return false;
                          }
                      }
                      position = end;
                      return encoder->write(pcm, error);
                  },
                  isCanceled,
                  error,
                  &frames)
        && opened && encoder->finish(error);
    if (ok && !sources.empty() && sources.back().end > position) {
        ok = false;
        *error = Text::translate("Audex::ImageRepair", "the audio read again reaches beyond the end of the image");
    }
    if (!ok) {
        encoder->abort();
        QFile::remove(part);
        return false;
    }

    const QString suffix = QFileInfo(rq.path).suffix();
    Text tagError;
    if (rq.writeTags)
        Encoding::TagWriter::write(part, suffix, rq.tags, &tagError); // the old image had them; a failure is not fatal

    if (!replaceFile(part, rq.path)) {
        *error = Text::translate("Audex::ImageRepair", "replacing %1 failed; the complete image is %2")
                     .arg(QFileInfo(rq.path).fileName(), QFileInfo(part).fileName());
        return false;
    }
    return true;
}

}

namespace Audex
{

std::optional<QList<Rip::SegmentResult>> checksumImage(const ImageChecksumRequest &rq, Text *error, const std::function<bool()> &isCanceled)
{
    if (!rq.encoder || rq.segments.isEmpty()) {
        *error = Text::plain(u"nothing to read"_s);
        return std::nullopt;
    }
    // the image starts with the first segment; audio beyond it is not read
    const DiscAudioFile image{rq.path, rq.encoder, rq.segments.first().firstLba, rq.segments.last().lastLba};
    return checksumFiles({image}, rq.segments, rq.options, error, isCanceled);
}

// ---- reading the disc audio back from files ---------------------------------------

struct DiscAudioReader::Part {
    std::unique_ptr<Encoding::AudioDecoder> decoder;
    std::unique_ptr<ImageSectorReader> reader;
};

DiscAudioReader::DiscAudioReader(const QList<DiscAudioFile> &files)
    : m_files(files)
{
}

DiscAudioReader::~DiscAudioReader() = default;

bool DiscAudioReader::open(Text *error)
{
    m_parts.clear();
    for (const DiscAudioFile &file : std::as_const(m_files)) {
        auto part = std::make_unique<Part>();
        part->decoder = file.format ? file.format->createDecoder() : nullptr;
        if (!part->decoder) {
            *error = Text::translate("Audex::ImageRepair", "%1 cannot be read back.").arg(QFileInfo(file.path).fileName());
            return false;
        }
        if (!part->decoder->open(file.path, error))
            return false;
        part->reader = std::make_unique<ImageSectorReader>(*part->decoder, file.firstLba);
        m_parts.push_back(std::move(part));
    }
    return true;
}

Rip::SectorReadResult DiscAudioReader::read(int lba, int count)
{
    Rip::SectorReadResult r;
    r.audio = QByteArray(qsizetype(count) * Cdda::SectorBytes, '\0');
    r.sectors.resize(count);
    int i = 0;
    while (i < count) {
        const int at = lba + i;
        qsizetype f = 0;
        while (f < m_files.size() && (at < m_files.at(f).firstLba || at > m_files.at(f).lastLba))
            ++f;
        if (f == m_files.size() || size_t(f) >= m_parts.size()) {
            ++i; // in no file: not ok
            continue;
        }
        const int n = std::min(count - i, m_files.at(f).lastLba - at + 1);
        const Rip::SectorReadResult part = m_parts.at(size_t(f))->reader->read(at, n);
        std::memcpy(r.audio.data() + qsizetype(i) * Cdda::SectorBytes, part.audio.constData(), size_t(n) * Cdda::SectorBytes);
        for (int k = 0; k < n; ++k)
            r.sectors[i + k] = part.sectors.value(k);
        if (!part.error.isEmpty())
            m_error = m_parts.at(size_t(f))->reader->error();
        i += n;
    }
    if (!r.allOk())
        r.error = m_error.isEmpty() ? u"beyond the decoded audio"_s : m_error.toEnglish();
    return r;
}

int DiscAudioReader::maxSectorsPerRead() const
{
    return 64;
}

QString DiscAudioReader::description() const
{
    return u"audio files"_s;
}

Text DiscAudioReader::error() const
{
    return m_error;
}

std::optional<QList<Rip::SegmentResult>> checksumFiles(const QList<DiscAudioFile> &files,
                                                       const QList<Rip::Segment> &segments,
                                                       const Rip::RipOptions &ripOptions,
                                                       Text *error,
                                                       const std::function<bool()> &isCanceled)
{
    if (files.isEmpty() || segments.isEmpty()) {
        *error = Text::plain(u"nothing to read"_s);
        return std::nullopt;
    }
    DiscAudioReader reader(files);
    if (!reader.open(error))
        return std::nullopt;

    Rip::RipOptions options = ripOptions; // AccurateRip shifts, CTDB range
    options.mode = Rip::ReadMode::Fast;
    options.readOffset = 0; // the files are corrected already
    options.overread = false;
    options.burstSectors = 0;
    options.readErrorRetries = 0;
    options.useC2 = false;
    options.cacheDefeat = false;
    options.readSpeed = 0;
    options.errorReadSpeed = 0;
    options.verifyFirst = false;

    Rip::RipCallbacks callbacks;
    callbacks.write = [](int, QByteArrayView) {
        return true;
    };
    callbacks.isCanceled = isCanceled;
    const Rip::ReadableArea area{files.first().firstLba, files.last().lastLba + 1};
    Rip::RipEngine engine(reader, area, options, std::move(callbacks));
    const Rip::RipResult result = engine.run(segments);
    if (!result.completed || result.hasSuspiciousPositions()) {
        *error = result.canceled ? Text::translate("Audex::ImageRepair", "canceled")
                                 : (!reader.error().isEmpty() ? reader.error() : Text::translate("Audex::ImageRepair", "the files are shorter than the disc"));
        return std::nullopt;
    }
    return result.segments;
}

}
