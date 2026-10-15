/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderwidget.h"

#include "dialogs/cddaheaderdatadialog.h"
#include "dialogs/coverchooserdialog.h"
#include "online/chosencovers.h"

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QHelpEvent>
#include <QImage>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QStandardPaths>
#include <QStyle>
#include <QTemporaryDir>
#include <QTextLayout>
#include <QToolTip>

#include <cmath>
#include <numbers>

using namespace Qt::StringLiterals;

namespace
{

void scaleFont(QFont &font, qreal factor)
{
    if (font.pixelSize() == -1)
        font.setPointSizeF(font.pointSizeF() * factor);
    else
        font.setPixelSize(qRound(font.pixelSize() * factor));
}

// Wraps text at word boundaries into at most maxLines lines, the last one
// elided; returns the y below the text
int drawWrappedText(QPainter &painter, const QString &text, const QFont &font, int x, int y, int width, int maxLines)
{
    const QFontMetrics fm(font);
    painter.setFont(font);
    QTextLayout layout(text, font);
    layout.beginLayout();
    for (int lines = 1;; ++lines) {
        QTextLine line = layout.createLine();
        if (!line.isValid())
            break;
        line.setLineWidth(width);
        QString part = text.mid(line.textStart(), line.textLength());
        if (lines == maxLines)
            part = fm.elidedText(text.mid(line.textStart()), Qt::ElideRight, width);
        painter.drawText(x, y + fm.ascent(), part.trimmed());
        y += fm.height();
        if (lines == maxLines)
            break;
    }
    layout.endLayout();
    return y;
}

// where construct_cd_case() draws the cover into the case image
constexpr QRectF CaseCover(125, 15, 1110, 1080);

// One period of the dots: each lights up in turn, then all rest a little
constexpr int DotsPeriodMs = 1200;

}

CDDAHeaderWidget ::CDDAHeaderWidget(Audex::CDInfoModel *cddaModel, QWidget *parent, const int cover_size_min, const int cover_size_max, const int padding)
    : QWidget(parent)
{
    cdda_model = cddaModel;
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    connect(cdda_model, &Audex::CDInfoModel::modelReset, this, &CDDAHeaderWidget::update);
    connect(cdda_model, &Audex::CDInfoModel::coverChanged, this, &CDDAHeaderWidget::update);
    connect(cdda_model, &Audex::CDInfoModel::albumChanged, this, &CDDAHeaderWidget::update);

    setup_actions();

    // a cover set by any means ends "nothing found" or "failed"
    connect(cdda_model, &Audex::CDInfoModel::coverChanged, this, [this]() {
        if (cdda_model && !cdda_model->coverImage().isNull() && m_coverState != CoverState::Loading)
            setCoverState(CoverState::Idle);
    });
    m_dotsTimer.setInterval(50);
    connect(&m_dotsTimer, &QTimer::timeout, this, [this]() {
        // QWidget::update(): this class's update() would build the case anew
        QWidget::update(coverArea().toAlignedRect().adjusted(-1, -1, 1, 1));
    });

    this->cover_size_min = cover_size_min;
    this->cover_size_max = cover_size_max;
    this->padding = padding;

    setMouseTracking(true);

    connect(this, &QWidget::customContextMenuRequested, this, &CDDAHeaderWidget::context_menu);

    setContextMenuPolicy(Qt::CustomContextMenu);

    setMinimumSize(QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2)));

    update();
}

CDDAHeaderWidget::~CDDAHeaderWidget()
{
    delete action_collection;
}

QSize CDDAHeaderWidget::sizeHint() const
{
    return QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2));
}

void CDDAHeaderWidget::setEnabled(bool enabled)
{
    this->enabled = enabled;
    repaint();
}

void CDDAHeaderWidget::paintEvent(QPaintEvent *event)
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::paintEvent() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    Q_UNUSED(event);

    QPainter painter;

    painter.begin(this);

    if (enabled) {
        const bool vertical = this->frameGeometry().width() > this->frameGeometry().height();
        const QSize caseSize = vertical ? QSize(qMin(rect().height() - padding * 2, cover_size_max), qMin(rect().height() - padding * 2, cover_size_max))
                                        : QSize(qMin(rect().width() - padding * 2, cover_size_max), qMin(rect().width() - padding * 2, cover_size_max));
        // scaled once per case, size and screen: the dots repaint the cover
        // often. In device pixels, so the cover stays sharp on scaled screens.
        const qreal dpr = devicePixelRatioF();
        if (m_scaledCaseKey != cd_case.cacheKey() || m_scaledCaseSize != caseSize || m_scaledCaseDpr != dpr) {
            m_scaledCase = cd_case.isNull() ? QImage() : cd_case.scaled(caseSize * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_scaledCase.setDevicePixelRatio(dpr);
            m_scaledCaseKey = cd_case.cacheKey();
            m_scaledCaseSize = caseSize;
            m_scaledCaseDpr = dpr;
        }
        const QImage &scaled_cd_case = m_scaledCase;
        const QSize caseLogicalSize = scaled_cd_case.deviceIndependentSize().toSize();

        int xOffsetCover = padding;
        int yOffsetCover = padding;

        int x = xOffsetCover;
        int y = yOffsetCover;

        // QImage scaled_cover = cover.scaled(cover_size, cover_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        cover_rect = QRect(QPoint(x, y), caseLogicalSize);
        painter.drawImage(QPoint(x, y), scaled_cd_case);
        paintCoverState(painter);

        painter.setRenderHint(QPainter::Antialiasing);

        // below the cover, as wide as the cover: album, artist, a line of
        // facts, the badges
        const int textWidth = qMax(caseLogicalSize.width(), 1);
        y += caseLogicalSize.height() + padding;
        const int yOffsetLabel = y;

        const QColor textColor = palette().color(QPalette::WindowText);
        const QColor secondaryColor = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::InactiveText).color();

        QFont albumFont = font();
        albumFont.setBold(true);
        scaleFont(albumFont, 1.3);
        QFont artistFont = font();
        scaleFont(artistFont, 1.1);
        QFont factsFont = font();
        scaleFont(factsFont, 0.9);

        const QString album = cdda_model->album();
        const QString artist = cdda_model->artist();
        painter.setPen(album.isEmpty() ? secondaryColor : textColor);
        y = drawWrappedText(painter, album.isEmpty() ? i18n("Unknown Album") : album, albumFont, x, y, textWidth, 2);
        y += 2;
        painter.setPen(artist.isEmpty() ? secondaryColor : textColor);
        y = drawWrappedText(painter, artist.isEmpty() ? i18n("Unknown Artist") : artist, artistFont, x, y, textWidth, 1);

        const QString facts = factsText();
        if (!facts.isEmpty()) {
            y += 4;
            painter.setPen(secondaryColor);
            y = drawWrappedText(painter, facts, factsFont, x, y, textWidth, 1);
        }

        // label_rect: where a click edits the data (not the badges)
        label_rect = QRect(x, yOffsetLabel, textWidth, y - yOffsetLabel);

        // flags of the disc as small badges in one row; their colors do not
        // depend on the color scheme, so the text is always white
        struct Badge {
            QString text;
            QColor fill;
            QColor border; // invalid: none
        };
        QList<Badge> badges;
        if (m_hdcd.value_or(false))
            badges.append({QStringLiteral("HDCD"), palette().accent().color(), QColor()});
        if (!cdda_model->cdInfo().preEmphasisTracks().isEmpty())
            badges.append({i18nc("badge in the disc header", "PRE-EMPHASIS"), QColor(0x8e, 0x44, 0xad), QColor()}); // purple
        if (m_cdg.value_or(false))
            badges.append({QStringLiteral("CD+G"), QColor(0x3b, 0x3f, 0x45), QColor(0x9a, 0xa0, 0xa6)}); // dark grey, visible on dark schemes too
        const Audex::CDInfo::Medium medium = cdda_model->cdInfo().medium();
        if (medium == Audex::CDInfo::Medium::CdR || medium == Audex::CDInfo::Medium::CdRw)
            badges.append({medium == Audex::CDInfo::Medium::CdRw ? QStringLiteral("CD-RW") : QStringLiteral("CD-R"), QColor(0x7f, 0x8c, 0x8d), QColor()}); // grey
        if (!badges.isEmpty()) {
            y += 8;
            const QFontMetrics fm(font());
            int badgeX = x;
            painter.save();
            painter.setFont(font());
            for (const Badge &badge : std::as_const(badges)) {
                const QRectF box(badgeX + 0.5, y + 0.5, fm.horizontalAdvance(badge.text) + 12, fm.height() + 4);
                painter.setBrush(badge.fill);
                painter.setPen(badge.border.isValid() ? QPen(badge.border, 1) : QPen(Qt::NoPen));
                painter.drawRoundedRect(box, 4, 4);
                painter.setPen(Qt::white);
                painter.drawText(box, Qt::AlignCenter, badge.text);
                badgeX = qRound(box.right()) + 6;
            }
            painter.restore();
        }

    } else { // disabled

        QFont font(QApplication::font());
        if (font.pixelSize() == -1) {
            font.setPointSizeF(font.pointSizeF() * 1.6);
        } else {
            font.setPixelSize(font.pixelSize() * 1.6);
        }
        font.setBold(true);
        font.setItalic(true);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter | Qt::AlignVCenter, i18n("No audio CD detected"));
    }

    painter.end();
}

// "1975 · Disc 1 of 2 · 9 tracks · 44:28"; what is not known is left out
QString CDDAHeaderWidget::factsText() const
{
    QStringList facts;
    if (!cdda_model->year().isEmpty())
        facts << cdda_model->year();
    const int disc = cdda_model->discNumber();
    const int discs = cdda_model->discCount();
    if (disc > 0 && discs > 1)
        facts << i18nc("disc number in the disc header", "Disc %1 of %2", disc, discs);
    else if (disc > 0)
        facts << i18nc("disc number in the disc header", "Disc %1", disc);

    const Audex::Cdda::Toc toc = cdda_model->cdInfo().toc();
    int tracks = 0;
    qint64 sectors = 0;
    for (const Audex::Cdda::Track &track : toc.tracks) {
        if (!track.audio)
            continue;
        ++tracks;
        sectors += track.sectorCount();
    }
    if (tracks > 0) {
        facts << i18np("%1 track", "%1 tracks", tracks);
        const qint64 seconds = (sectors + Audex::Cdda::SectorsPerSecond / 2) / Audex::Cdda::SectorsPerSecond;
        facts << (seconds >= 3600 ? u"%1:%2:%3"_s.arg(seconds / 3600).arg(seconds / 60 % 60, 2, 10, QLatin1Char('0')).arg(seconds % 60, 2, 10, QLatin1Char('0'))
                                  : u"%1:%2"_s.arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0')));
    }
    return facts.join(u" · "_s);
}

void CDDAHeaderWidget::construct_cd_case()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::construct_cd_case() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    QImage cdcase_wo_latches = QImage(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("audex/images/cdcase_wo_latches.png")));
    QImage latches = QImage(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QString("audex/images/latches.png")));
    QImage cdcase_covered = QImage(cdcase_wo_latches.size(), QImage::Format_ARGB32_Premultiplied);

    QPainter cover_painter(&cdcase_covered);
    cover_painter.setRenderHint(QPainter::Antialiasing);

    cover_painter.setCompositionMode(QPainter::CompositionMode_Source);
    cover_painter.fillRect(cdcase_covered.rect(), Qt::transparent);

    cover_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    cover_painter.drawImage(0, 0, cdcase_wo_latches);

    cover_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (!cdda_model->coverImage().isNull())
        cover_painter.drawImage(CaseCover.topLeft(), cdda_model->coverImage().scaled(CaseCover.size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    cover_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    cover_painter.drawImage(259, 0, latches);

    cover_painter.end();

    this->cd_case = cdcase_covered;
    this->case_cover = CaseCover;
}

bool CDDAHeaderWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && cdda_model) {
        const auto *help = static_cast<QHelpEvent *>(event);
        if (cover_rect.contains(help->pos())) {
            QToolTip::showText(help->globalPos(), coverToolTip(), this, cover_rect);
            return true;
        }
    }
    return QWidget::event(event);
}

QString CDDAHeaderWidget::coverToolTip() const
{
    if (m_coverState == CoverState::Loading)
        return i18n("Looking for a cover...");
    const Audex::Metadata::CoverArt cover = cdda_model->cover();
    if (cover.isNull() && m_coverState == CoverState::NotFound) {
        const QString where = m_coverStateDetail.isEmpty() ? i18n("No cover found.") : m_coverStateDetail;
        return u"<p>%1</p><p>%2</p>"_s.arg(where.toHtmlEscaped(), i18n("Click to set one from a file.").toHtmlEscaped());
    }
    if (cover.isNull() && m_coverState == CoverState::Failed)
        return u"<p>%1</p><p>%2</p>"_s.arg(i18n("The cover could not be loaded: %1", m_coverStateDetail).toHtmlEscaped(),
                                           i18n("Click to set one from a file, or use “Fetch Cover Again” in the context menu.").toHtmlEscaped());
    if (cover.isNull())
        return i18n("No cover. Click to set one from a file.");

    QString origin = cover.origin;
    if (origin.isEmpty() && QDir::isAbsolutePath(cover.source))
        origin = i18n("From the file %1.", cover.source);
    else if (origin.isEmpty() && !cover.source.isEmpty())
        origin = i18n("From %1.", cover.source);
    else if (origin.isEmpty())
        origin = i18n("Origin unknown.");

    const QImage image = cdda_model->coverImage();
    const QString format = cover.suffix().toUpper();
    const QString size = QLocale().formattedDataSize(cover.data.size());
    const QString details = image.isNull() ? i18n("%1, %2", format, size) : i18n("%1 × %2 pixels, %3, %4", image.width(), image.height(), format, size);
    return u"<p>%1</p><p>%2</p>"_s.arg(origin.toHtmlEscaped(), details.toHtmlEscaped());
}

void CDDAHeaderWidget::choose_cover()
{
    if (!cdda_model)
        return;
    CoverChooserDialog dialog(cdda_model->cdInfo().metadata(), this);
    if (dialog.exec() == QDialog::Accepted) {
        cdda_model->setCover(dialog.cover());
        remember_cover();
    }
}

// A cover the user chose comes with every later lookup of the disc
void CDDAHeaderWidget::remember_cover()
{
    Q_EMIT coverSetByUser(); // a download still on its way must not replace it
    std::optional<Audex::ResponseCache> cache = onlineCache();
    if (!cache)
        return;
    const QString discId = cdda_model->cdInfo().musicBrainzDiscId();
    if (cdda_model->cover().isNull())
        Audex::ChosenCovers::forget(*cache, discId);
    else
        Audex::ChosenCovers::keep(*cache, discId, cdda_model->cover());
}

void CDDAHeaderWidget::show_cover_source()
{
    if (cdda_model && cdda_model->cover().originPage.isValid())
        QDesktopServices::openUrl(cdda_model->cover().originPage);
}

bool CDDAHeaderWidget::onCover(const QPoint &pos) const
{
    return enabled && cover_rect.contains(pos);
}

bool CDDAHeaderWidget::onLabel(const QPoint &pos) const
{
    return enabled && label_rect.contains(pos);
}

// The widget's own cursor: it ends with the widget, whatever the mouse does
void CDDAHeaderWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (onCover(event->pos()) || onLabel(event->pos()))
        setCursor(Qt::PointingHandCursor);
    else
        unsetCursor();
}

void CDDAHeaderWidget::mousePressEvent(QMouseEvent *event)
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::mousePressEvent() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (onCover(event->pos())) {
            unsetCursor();
            if (cdda_model->cover().isNull()) {
                load();
            } else {
                view_cover();
            }
        } else if (onLabel(event->pos())) {
            unsetCursor();
            edit_data();
        }
    }
}

void CDDAHeaderWidget::update()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::update() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    bool activate = false;
    if (!cdda_model->coverImage().isNull()) {
        activate = true;
    }

    action_collection->action("save")->setEnabled(activate);
    action_collection->action("view")->setEnabled(activate);
    action_collection->action("source")->setEnabled(activate && cdda_model->cover().originPage.isValid());
    action_collection->action("remove")->setEnabled(activate);

    construct_cd_case();
    repaint();
}

void CDDAHeaderWidget::load()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::load() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    QString filename = QFileDialog::getOpenFileName(this, i18n("Load Cover"), QDir::homePath(), i18n("Images (*.jpg *.jpeg *.png *.gif *.webp *.bmp)"));
    if (!filename.isEmpty()) {
        QString error;
        if (!cdda_model->loadCoverFromFile(filename, &error)) {
            ErrorDialog::show(this, i18n("Cannot load the cover."), error);
            return;
        }
        remember_cover();
    }
}

void CDDAHeaderWidget::save()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::save() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    QString filename = QFileDialog::getSaveFileName(this, i18n("Save Cover"), QDir::homePath() + '/' + cdda_model->album() + ".jpg");
    if (!filename.isEmpty()) {
        QString error;
        if (!cdda_model->saveCoverToFile(filename, &error)) {
            ErrorDialog::show(this, i18n("Cannot save the cover."), error);
        }
    }
}

void CDDAHeaderWidget::view_cover()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::view_cover() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    QString filename = tmp_dir.filePath(QString("%1.jpeg").arg(cdda_model->coverImage().cacheKey(), 8, 16));
    cdda_model->saveCoverToFile(filename);

    qDebug() << "Open" << filename;

    QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
}

void CDDAHeaderWidget::remove()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::remove() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    cdda_model->removeCover();
    remember_cover(); // forgets it
    update();
}

void CDDAHeaderWidget::edit_data()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::edit_data() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    auto dialog(CDDAHeaderDataDialog(cdda_model, m_hdcd, this));

    if (dialog.exec() != QDialog::Accepted)
        return;

    update();
    Q_EMIT headerDataChanged();
}

void CDDAHeaderWidget::setHdcd(std::optional<bool> detected)
{
    m_hdcd = detected;
    // called for every new disc as well: its pre-emphasis flags come along
    updateToolTip();
    update();
}

QString CDDAHeaderWidget::preEmphasisText(const Audex::CDInfo &info)
{
    const QList<int> tracks = info.preEmphasisTracks();
    if (tracks.isEmpty())
        return QString();
    const QString hint = i18n(
        "The audio was mastered with raised treble and has to be de-emphasized for playback. The rip keeps it unchanged; see the README for de-emphasis with "
        "FFmpeg.");
    if (tracks.size() == info.audioTrackNumbers().size())
        return i18n("Pre-emphasis on all tracks.") + u' ' + hint;
    QStringList numbers;
    for (const int number : tracks)
        numbers << QString::number(info.displayTrackNumber(number));
    return i18np("Pre-emphasis on track %2.", "Pre-emphasis on tracks %2.", tracks.size(), numbers.join(QStringLiteral(", "))) + u' ' + hint;
}

void CDDAHeaderWidget::context_menu(const QPoint &point)
{
    qDebug() << "context menu requested at point" << point;
    if (onCover(point)) {
        unsetCursor();
        QMenu contextMenu(this);
        contextMenu.clear();
        if (m_coverState == CoverState::Failed) {
            contextMenu.addAction(action_collection->action("refetch"));
            contextMenu.addSeparator();
        }
        action_collection->action("choose")->setEnabled(CoverChooserDialog::canChoose(cdda_model->cdInfo().metadata()));
        contextMenu.addAction(action_collection->action("choose"));
        contextMenu.addAction(action_collection->action("load"));
        contextMenu.addAction(action_collection->action("save"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("view"));
        contextMenu.addAction(action_collection->action("source"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("remove"));
        contextMenu.exec(mapToGlobal(point));
    }
}

void CDDAHeaderWidget::setCoverState(CoverState state, const QString &detail)
{
    m_coverState = state;
    m_coverStateDetail = detail;

    // no animation if the user turned animations off (Plasma: animation speed "instant")
    const bool animate = state == CoverState::Loading && style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) > 0;
    if (animate) {
        m_dotsClock.restart();
        m_dotsTimer.start();
    } else {
        m_dotsTimer.stop();
    }
    action_collection->action("refetch")->setEnabled(state == CoverState::Failed);
    QWidget::update(); // the case itself is unchanged
}

QRectF CDDAHeaderWidget::coverArea() const
{
    if (cd_case.isNull() || cover_rect.isEmpty() || case_cover.isEmpty())
        return QRectF(cover_rect);
    const qreal scale = qreal(cover_rect.width()) / qreal(cd_case.width());
    const QRectF area(cover_rect.x() + case_cover.x() * scale, cover_rect.y() + case_cover.y() * scale, case_cover.width() * scale, case_cover.height() * scale);
    return area.intersected(QRectF(cover_rect));
}

void CDDAHeaderWidget::paintCoverState(QPainter &painter)
{
    if (m_coverState == CoverState::Idle || cover_rect.isEmpty())
        return;
    // a cover that is already there stays visible; the tool tip tells the rest
    if (m_coverState != CoverState::Loading && !cdda_model->coverImage().isNull())
        return;

    const QRectF area = coverArea();
    QColor plate = palette().color(QPalette::Window);
    plate.setAlpha(215);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    if (m_coverState == CoverState::Loading) {
        const qreal dot = qBound(6.0, area.width() * 0.045, 14.0);
        const qreal gap = dot * 0.9;
        const QSizeF plateSize(3 * dot + 2 * gap + 2 * dot, 2.4 * dot);
        const QRectF plateRect(area.center() - QPointF(plateSize.width() / 2, plateSize.height() / 2), plateSize);
        painter.setBrush(plate);
        painter.drawRoundedRect(plateRect, plateSize.height() / 2, plateSize.height() / 2);

        // each dot lights up for 40 % of the period, the next one a little later
        const bool animated = m_dotsTimer.isActive();
        const qreal t = animated ? qreal(m_dotsClock.elapsed() % DotsPeriodMs) / DotsPeriodMs : 0.0;
        for (int i = 0; i < 3; ++i) {
            qreal level = 0.55; // static without animations
            if (animated) {
                qreal phase = t - i * 0.18;
                phase -= std::floor(phase);
                const qreal pulse = phase < 0.4 ? std::sin(std::numbers::pi * phase / 0.4) : 0.0;
                level = 0.25 + 0.75 * pulse;
            }
            QColor color = palette().color(QPalette::WindowText);
            color.setAlphaF(float(level));
            painter.setBrush(color);
            const QPointF center(plateRect.left() + dot + dot / 2 + i * (dot + gap), plateRect.center().y());
            painter.drawEllipse(center, dot / 2, dot / 2);
        }
    } else {
        const QString text = m_coverState == CoverState::NotFound ? i18n("No cover found") : i18n("Cover could not be loaded");
        QColor color;
        if (m_coverState == CoverState::NotFound) {
            color = palette().color(QPalette::WindowText);
            color.setAlpha(170); // muted: nothing went wrong
        } else {
            color = KColorScheme(QPalette::Active, KColorScheme::Window).foreground(KColorScheme::NegativeText).color();
        }
        QFont font = painter.font();
        font.setPointSizeF(font.pointSizeF() * 0.9);
        painter.setFont(font);
        const QFontMetricsF fm(font);
        const qreal maxWidth = area.width() * 0.8;
        const QRectF textRect = fm.boundingRect(QRectF(0, 0, maxWidth, area.height()), Qt::AlignCenter | Qt::TextWordWrap, text);
        const QSizeF plateSize(textRect.width() + fm.height(), textRect.height() + fm.height() * 0.6);
        const QRectF plateRect(area.center() - QPointF(plateSize.width() / 2, plateSize.height() / 2), plateSize);
        painter.setBrush(plate);
        painter.drawRoundedRect(plateRect, fm.height() / 2, fm.height() / 2);
        painter.setPen(color);
        painter.drawText(plateRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
    painter.restore();
}

void CDDAHeaderWidget::setup_actions()
{
    action_collection = new KActionCollection(this);

    auto *chooseCoverAction = new QAction(this);
    chooseCoverAction->setText(i18n("Choose Cover..."));
    chooseCoverAction->setToolTip(i18n("Shows all images the Cover Art Archive has for this album (needs MusicBrainz metadata)."));
    action_collection->addAction("choose", chooseCoverAction);
    connect(chooseCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::choose_cover);

    auto *refetchCoverAction = new QAction(this);
    refetchCoverAction->setText(i18n("Fetch Cover Again"));
    refetchCoverAction->setEnabled(false);
    action_collection->addAction("refetch", refetchCoverAction);
    connect(refetchCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::coverFetchRequested);

    auto *loadCoverAction = new QAction(this);
    loadCoverAction->setText(i18n("Set Custom Cover..."));
    action_collection->addAction("load", loadCoverAction);
    connect(loadCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::load);

    auto *saveCoverAction = new QAction(this);
    saveCoverAction->setText(i18n("Save Cover To File..."));
    action_collection->addAction("save", saveCoverAction);
    connect(saveCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::save);

    auto *viewCoverAction = new QAction(this);
    viewCoverAction->setText(i18n("Show Full Size Cover..."));
    action_collection->addAction("view", viewCoverAction);
    connect(viewCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::view_cover);

    auto *sourceCoverAction = new QAction(this);
    sourceCoverAction->setText(i18n("Show Cover Source"));
    sourceCoverAction->setToolTip(i18n("Opens the page the cover comes from, e.g. the cover art of the release at MusicBrainz."));
    action_collection->addAction("source", sourceCoverAction);
    connect(sourceCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::show_cover_source);

    auto *removeCoverAction = new QAction(this);
    removeCoverAction->setText(i18n("Remove Cover"));
    action_collection->addAction("remove", removeCoverAction);
    connect(removeCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::remove);
}

void CDDAHeaderWidget::setCdg(std::optional<bool> detected)
{
    m_cdg = detected;
    updateToolTip();
    update();
}

void CDDAHeaderWidget::updateToolTip()
{
    QStringList tips;
    if (m_hdcd.value_or(false))
        tips << i18n("HDCD encoded disc. The bit-perfect rip preserves all HDCD information; see the README for decoding with FFmpeg.");
    if (m_cdg.value_or(false))
        tips << i18n("CD+G disc: graphics in the sub-channel. Turn on \"Save CD+G graphics as .cdg files\" in the general settings to keep them.");
    if (cdda_model) {
        const Audex::CDInfo &info = cdda_model->cdInfo();
        if (info.medium() == Audex::CDInfo::Medium::CdR || info.medium() == Audex::CDInfo::Medium::CdRw) {
            const QString kind = info.medium() == Audex::CDInfo::Medium::CdRw ? QStringLiteral("CD-RW") : QStringLiteral("CD-R");
            QString tip = info.mediumDetails().isEmpty() ? i18n("Recordable disc (%1).", kind) : i18n("Recordable disc (%1, %2).", kind, info.mediumDetails());
            tip += u' '
                + i18n("Usually a copy: AccurateRip confirms it only if it was burned bit-exact from a pressing. Burned discs age and can be harder to read.");
            tips << tip;
        }
        const QString emphasis = preEmphasisText(cdda_model->cdInfo());
        if (!emphasis.isEmpty())
            tips << emphasis;
    }
    setToolTip(tips.join(QStringLiteral("\n\n")));
}
