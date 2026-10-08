/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cdtext.h"

#include <QStringDecoder>
#include <QtEndian>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace Audex::Cdda
{

namespace
{

constexpr int PackSize = 18;
constexpr int PayloadSize = 12;

struct Pack {
    quint8 type = 0;
    quint8 track = 0; // id2 without extension flag
    quint8 sequence = 0;
    int block = 0;
    int charPosition = 0;
    bool doubleByte = false;
    QByteArray payload;
};

bool isTextPack(quint8 type)
{
    return (type >= CdTextPack::Title && type <= CdTextPack::DiscId) || type == CdTextPack::UpcIsrc;
}

class TextDecoder
{
public:
    TextDecoder(quint8 characterCode, QStringList &warnings)
        : m_code(characterCode)
    {
        if (m_code == CdTextCharset::MsJis) {
            m_decoder = QStringDecoder("Shift_JIS");
            if (!m_decoder.isValid())
                warnings << u"MS-JIS (Shift_JIS) text cannot be decoded: no codec available"_s;
        } else if (m_code != CdTextCharset::Iso8859_1 && m_code != CdTextCharset::Ascii) {
            warnings << u"Unsupported CD-Text character code 0x%1, text ignored"_s.arg(m_code, 2, 16, QLatin1Char('0'));
        }
    }

    bool usable() const
    {
        return m_code == CdTextCharset::Iso8859_1 || m_code == CdTextCharset::Ascii || m_decoder.isValid();
    }

    QString decode(const QByteArray &bytes, bool asciiOnly)
    {
        if (asciiOnly || m_code == CdTextCharset::Iso8859_1 || m_code == CdTextCharset::Ascii)
            return QString::fromLatin1(bytes);
        return m_decoder.decode(bytes);
    }

private:
    quint8 m_code;
    QStringDecoder m_decoder;
};

void decodeTextPacks(CdTextBlock &block, quint8 type, const QList<Pack> &packs, QStringList &warnings)
{
    TextDecoder decoder(block.characterCode, warnings);
    // UPC/ISRC and disc id are always single byte ASCII
    const bool asciiOnly = type == CdTextPack::UpcIsrc || type == CdTextPack::DiscId;
    if (!asciiOnly && !decoder.usable())
        return;

    QMap<int, QString> &out = block.texts[type];
    QByteArray pending;
    int previousSequence = -1;

    auto emitString = [&](int track) {
        const bool doubleTab = pending == QByteArray("\t\t", 2);
        QString s;
        if (pending == "\t" || doubleTab)
            s = out.value(track - 1);
        else
            s = decoder.decode(pending, asciiOnly);
        pending.clear();
        if (!s.isEmpty() && !out.contains(track))
            out.insert(track, s);
    };

    for (const Pack &p : packs) {
        // A gap in the sequence (dropped pack) or an inconsistent character
        // position invalidates the string that was being assembled.
        const bool gap = previousSequence >= 0 && p.sequence != quint8(previousSequence + 1);
        if (gap || p.charPosition == 0)
            pending.clear();
        previousSequence = p.sequence;
        // The pack continues a string whose beginning is missing: skip its rest.
        bool discard = p.charPosition > 0 && pending.isEmpty();

        int track = p.track;
        const int step = (p.doubleByte && !asciiOnly) ? 2 : 1;
        for (int i = 0; i + step <= PayloadSize; i += step) {
            const bool terminator = step == 1 ? p.payload.at(i) == '\0' : (p.payload.at(i) == '\0' && p.payload.at(i + 1) == '\0');
            if (terminator) {
                if (discard)
                    discard = false;
                else
                    emitString(track);
                ++track;
            } else if (!discard) {
                pending.append(p.payload.constData() + i, step);
            }
        }
    }
}

}

quint16 cdTextCrc(QByteArrayView data)
{
    quint16 crc = 0;
    for (char c : data) {
        crc ^= quint16(quint8(c)) << 8;
        for (int b = 0; b < 8; ++b)
            crc = (crc & 0x8000) ? quint16((crc << 1) ^ 0x1021) : quint16(crc << 1);
    }
    return crc;
}

const CdTextBlock *CdText::preferredBlock(quint8 languageCode) const
{
    for (const CdTextBlock &b : blocks)
        if (b.hasSizeInfo && b.languageCode == languageCode)
            return &b;
    for (const CdTextBlock &b : blocks)
        if (b.block == 0)
            return &b;
    return blocks.isEmpty() ? nullptr : &blocks.first();
}

std::optional<CdText> parseCdText(QByteArrayView response, QString *error)
{
    auto fail = [error](const QString &msg) -> std::optional<CdText> {
        if (error)
            *error = msg;
        return std::nullopt;
    };

    if (response.size() < 4)
        return fail(u"CD-Text response too short"_s);
    const qsizetype dataLength = qFromBigEndian<quint16>(response.constData());
    const qsizetype packBytes = std::min<qsizetype>(response.size() - 4, dataLength - 2);
    if (packBytes < PackSize)
        return fail(u"No CD-Text on this disc"_s);

    CdText result;
    QList<Pack> packs;
    QList<bool> crcOk;
    for (qsizetype pos = 4; pos + PackSize <= 4 + packBytes; pos += PackSize) {
        const QByteArrayView raw = response.sliced(pos, PackSize);
        Pack p;
        p.type = quint8(raw.at(0));
        p.track = quint8(raw.at(1)) & 0x7F;
        p.sequence = quint8(raw.at(2));
        const quint8 b3 = quint8(raw.at(3));
        p.doubleByte = b3 & 0x80;
        p.block = (b3 >> 4) & 0x07;
        p.charPosition = b3 & 0x0F;
        p.payload = raw.sliced(4, PayloadSize).toByteArray();
        packs.append(p);

        const quint16 stored = qFromBigEndian<quint16>(raw.constData() + 16);
        crcOk.append(quint16(~cdTextCrc(raw.first(16))) == stored);
    }
    result.packCount = int(packs.size());

    const bool anyCrcOk = std::any_of(crcOk.cbegin(), crcOk.cend(), [](bool b) {
        return b;
    });
    if (!anyCrcOk) {
        result.crcIgnored = true;
        result.warnings << u"The drive delivered no valid CD-Text checksums; data used unchecked"_s;
    } else {
        QList<Pack> valid;
        for (qsizetype i = 0; i < packs.size(); ++i) {
            if (crcOk.at(i))
                valid.append(packs.at(i));
            else
                ++result.crcErrors;
        }
        packs = valid;
        if (result.crcErrors > 0)
            result.warnings << u"%1 CD-Text pack(s) dropped because of CRC errors"_s.arg(result.crcErrors);
    }

    for (int b = 0; b < 8; ++b) {
        QList<Pack> blockPacks;
        for (const Pack &p : std::as_const(packs))
            if (p.block == b && p.type >= 0x80 && p.type <= 0x8F)
                blockPacks.append(p);
        if (blockPacks.isEmpty())
            continue;
        std::stable_sort(blockPacks.begin(), blockPacks.end(), [](const Pack &x, const Pack &y) {
            return x.sequence < y.sequence;
        });

        CdTextBlock block;
        block.block = b;

        // size information: three packs, 36 bytes
        QByteArray size;
        for (const Pack &p : std::as_const(blockPacks))
            if (p.type == CdTextPack::SizeInfo)
                size += p.payload;
        if (size.size() >= 36) {
            block.hasSizeInfo = true;
            block.characterCode = quint8(size.at(0));
            block.firstTrack = quint8(size.at(1));
            block.lastTrack = quint8(size.at(2));
            block.languageCode = quint8(size.at(28 + b));
        }

        QMap<quint8, QList<Pack>> byType;
        for (const Pack &p : std::as_const(blockPacks))
            byType[p.type].append(p);

        for (auto it = byType.cbegin(); it != byType.cend(); ++it) {
            if (isTextPack(it.key()))
                decodeTextPacks(block, it.key(), it.value(), result.warnings);
        }

        // genre: 2 byte code followed by a supplementary text
        QByteArray genre;
        for (const Pack &p : byType.value(CdTextPack::Genre))
            if (p.track == 0)
                genre += p.payload;
        if (genre.size() >= 2) {
            block.genreCode = qFromBigEndian<quint16>(genre.constData());
            QByteArray text = genre.mid(2);
            const qsizetype nul = text.indexOf('\0');
            if (nul >= 0)
                text.truncate(nul);
            block.genreText = QString::fromLatin1(text).trimmed();
        }

        // drop empty type maps
        for (auto t = block.texts.begin(); t != block.texts.end();) {
            if (t.value().isEmpty())
                t = block.texts.erase(t);
            else
                ++t;
        }

        if (!block.texts.isEmpty() || block.genreCode > 1 || !block.genreText.isEmpty())
            result.blocks.append(block);
    }

    if (result.blocks.isEmpty())
        return fail(u"CD-Text contains no usable text"_s);
    return result;
}

QString cdTextGenreName(int code)
{
    static const char *const names[] = {
        nullptr,
        nullptr,
        "Adult Contemporary",
        "Alternative Rock",
        "Childrens' Music",
        "Classical",
        "Contemporary Christian",
        "Country",
        "Dance",
        "Easy Listening",
        "Erotic",
        "Folk",
        "Gospel",
        "Hip Hop",
        "Jazz",
        "Latin",
        "Musical",
        "New Age",
        "Opera",
        "Operetta",
        "Pop Music",
        "Rap",
        "Reggae",
        "Rock Music",
        "Rhythm & Blues",
        "Sound Effects",
        "Spoken Word",
        "World Music",
    };
    if (code < 0 || code >= int(std::size(names)) || !names[code])
        return QString();
    return QString::fromLatin1(names[code]);
}

QString cdTextLanguageName(quint8 code)
{
    switch (code) {
    case 0x00: return u"unknown"_s;
    case 0x08: return u"German"_s;
    case 0x09: return u"English"_s;
    case 0x0A: return u"Spanish"_s;
    case 0x0F: return u"French"_s;
    case 0x15: return u"Italian"_s;
    case 0x1D: return u"Dutch"_s;
    case 0x56: return u"Russian"_s;
    case 0x65: return u"Korean"_s;
    case 0x69: return u"Japanese"_s;
    case 0x75: return u"Chinese"_s;
    }
    return u"0x%1"_s.arg(code, 2, 16, QLatin1Char('0'));
}

}
