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

QString tr(const char *text)
{
    return QCoreApplication::translate("Audex::ImageRepair", text);
}

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
                QString *error,
                qint64 *frames = nullptr)
{
    const std::unique_ptr<Encoding::AudioDecoder> decoder = factory.createDecoder();
    if (!decoder) {
        *error = tr("%1 files cannot be read back.").arg(factory.displayName());
        return false;
    }
    if (!decoder->open(path, error))
        return false;
    if (frames)
        *frames = decoder->totalFrames();
    QByteArray pcm;
    while (true) {
        if (isCanceled && isCanceled()) {
            *error = tr("canceled");
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
            r.error = m_error.isEmpty() ? u"beyond the decoded audio"_s : m_error;
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

    QString error() const
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
    QString m_error;
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
    result.log << u"CUETools database (CTDB) repair"_s << QString();
    const auto note = [&](const QString &line) {
        result.log << u"     "_s + line;
    };
    const auto say = [&](const QString &text) {
        if (message)
            message(text);
    };

    const Cdda::Track *first = rq.toc.track(rq.toc.firstAudioTrackNumber());
    const Cdda::Track *last = rq.toc.track(rq.toc.lastAudioTrackNumber());
    if (!rq.encoder || !first || !last || first->firstLba < rq.firstLba) {
        note(u"Not possible for this image."_s);
        return result;
    }
    const qint64 offsetWords = qint64(first->firstLba - rq.firstLba) * WordsPerSector;
    Ctdb::Parity parity(offsetWords, qint64(last->lastLba + 1 - first->firstLba) * WordsPerSector);
    if (!parity.isValid()) {
        note(u"Not possible: the disc is too short or too long."_s);
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
        note(u"The database has no recovery data for this disc."_s);
        return result;
    }

    // --- read the image back ---
    say(tr("Reading the image for the repair..."));
    QString error;
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
        note(u"Reading %1 failed: %2"_s.arg(QFileInfo(rq.path).fileName(), error.isEmpty() ? u"the image is too short"_s : error));
        return result;
    }

    // --- an entry the image can be restored to ---
    const auto fetch = rq.fetchParity ? rq.fetchParity : [&isCanceled](const Ctdb::Entry &e, QString *err) {
        return Ctdb::fetchParity(e, err, 60000, isCanceled);
    };
    const Ctdb::Entry *chosen = nullptr;
    int shift = 0;
    QList<Ctdb::Correction> corrections;
    QStringList skipped;
    for (const Ctdb::Entry &e : std::as_const(candidates)) {
        const QString name = u"[%1] (confidence %2)"_s.arg(hex8(e.crc)).arg(e.confidence);
        const std::optional<Ctdb::ShiftMatch> match = parity.findShift(e.syndromes, e.crc);
        if (!match) {
            skipped << u"%1: too different"_s.arg(name);
            continue;
        }
        if (!match->errors) {
            note(u"The image matches %1, nothing to repair."_s.arg(name));
            return result;
        }
        if (!e.hasParity()) {
            skipped << u"%1: no recovery data"_s.arg(name);
            continue;
        }
        say(tr("Downloading the recovery data of the CUETools database..."));
        QString fetchError;
        const QByteArray data = fetch(e, &fetchError);
        if (data.size() < e.syndromes.size() * Ctdb::Stride * 2) {
            skipped << u"%1: download failed (%2)"_s.arg(name, fetchError);
            continue;
        }
        bool consistent = true; // the recovery data belongs to the entry
        for (int i = 0; i < e.syndromes.size(); ++i)
            consistent = consistent && qFromLittleEndian<quint16>(data.constData() + qsizetype(i) * Ctdb::Stride * 2) == e.syndromes.at(i);
        if (!consistent) {
            skipped << u"%1: recovery data does not fit the entry"_s.arg(name);
            continue;
        }
        const auto fix = parity.corrections(data, int(e.syndromes.size()), match->shift, e.crc);
        if (!fix) {
            skipped << u"%1: too much damage"_s.arg(name);
            continue;
        }
        chosen = &e;
        shift = match->shift;
        corrections = *fix;
        break;
    }
    for (const QString &line : std::as_const(skipped))
        note(line);
    if (!chosen) {
        note(u"The image could not be repaired."_s);
        return result;
    }
    note(u"Entry [%1] (confidence %2)%3"_s.arg(hex8(chosen->crc))
             .arg(chosen->confidence)
             .arg(shift == 0 ? QString() : u", other pressing with offset %1%2"_s.arg(shift > 0 ? u"+"_s : QString()).arg(shift)));
    note(u"Corrections: %1 values at %2"_s.arg(corrections.size()).arg(positions(corrections, rq.firstLba)));

    // --- write the repaired image ---
    say(tr("Writing the repaired image..."));
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
        error = u"the repaired audio does not match the entry"_s;
    }
    if (!ok) {
        encoder->abort();
        QFile::remove(part);
        note(u"Writing the repaired image failed: %1"_s.arg(error));
        return result;
    }

    const QString suffix = QFileInfo(rq.path).suffix();
    if (rq.writeTags && Encoding::TagWriter::supportsSuffix(suffix) && !Encoding::TagWriter::write(part, suffix, rq.tags, &error))
        note(u"Tagging the repaired image failed: %1"_s.arg(error));

    if (rq.keepOriginal) {
        result.originalPath = unrepairedPath(rq.path);
        QFile::remove(result.originalPath);
        ok = QFile::rename(rq.path, result.originalPath);
    } else {
        ok = QFile::remove(rq.path);
    }
    if (!ok || !QFile::rename(part, rq.path)) {
        note(u"Replacing %1 failed; the repaired image is %2"_s.arg(QFileInfo(rq.path).fileName(), QFileInfo(part).fileName()));
        return result;
    }

    result.repaired = true;
    result.corrections = int(corrections.size());
    note(u"Repaired: the image matches the database entry now."_s);
    if (rq.keepOriginal)
        note(u"The unrepaired image was kept as %1"_s.arg(QFileInfo(result.originalPath).fileName()));
    return result;
}

bool patchImage(const ImagePatchRequest &rq, QString *error, const std::function<bool()> &isCanceled)
{
    if (!rq.encoder) {
        *error = u"no encoder"_s;
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
            *error = u"%1: %2"_s.arg(patch.pcmPath, file->errorString());
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
                              *error = u"%1: %2"_s.arg(source.file->fileName(), source.file->errorString());
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
        *error = u"the audio read again reaches beyond the end of the image"_s;
    }
    if (!ok) {
        encoder->abort();
        QFile::remove(part);
        return false;
    }

    const QString suffix = QFileInfo(rq.path).suffix();
    QString tagError;
    if (rq.writeTags && Encoding::TagWriter::supportsSuffix(suffix))
        Encoding::TagWriter::write(part, suffix, rq.tags, &tagError); // the old image had them; a failure is not fatal

    if (!QFile::remove(rq.path) || !QFile::rename(part, rq.path)) {
        *error = u"replacing %1 failed; the complete image is %2"_s.arg(QFileInfo(rq.path).fileName(), QFileInfo(part).fileName());
        return false;
    }
    return true;
}

}

namespace Audex
{

std::optional<QList<Rip::SegmentResult>> checksumImage(const ImageChecksumRequest &rq, QString *error, const std::function<bool()> &isCanceled)
{
    if (!rq.encoder || rq.segments.isEmpty()) {
        *error = u"nothing to read"_s;
        return std::nullopt;
    }
    const std::unique_ptr<Encoding::AudioDecoder> decoder = rq.encoder->createDecoder();
    if (!decoder) {
        *error = tr("%1 files cannot be read back.").arg(rq.encoder->displayName());
        return std::nullopt;
    }
    if (!decoder->open(rq.path, error))
        return std::nullopt;

    ImageSectorReader reader(*decoder, rq.segments.first().firstLba);
    Rip::RipOptions options = rq.options; // AccurateRip shifts, CTDB range
    options.mode = Rip::ReadMode::Fast;
    options.readOffset = 0; // the image is corrected already
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
    const Rip::ReadableArea area{rq.segments.first().firstLba, rq.segments.last().lastLba + 1};
    Rip::RipEngine engine(reader, area, options, std::move(callbacks));
    const Rip::RipResult result = engine.run(rq.segments);
    if (!result.completed || result.hasSuspiciousPositions()) {
        *error = result.canceled ? tr("canceled") : (!reader.error().isEmpty() ? reader.error() : tr("the image is shorter than the disc"));
        return std::nullopt;
    }
    return result.segments;
}

}
