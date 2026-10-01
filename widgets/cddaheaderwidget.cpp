/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderwidget.h"

#include "dialogs/cddaheaderdatadialog.h"
#include "dialogs/coverchooserdialog.h"

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
#include <QTemporaryDir>
#include <QToolTip>

using namespace Qt::StringLiterals;

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

    this->cover_size_min = cover_size_min;
    this->cover_size_max = cover_size_max;
    this->padding = padding;

    setMouseTracking(true);
    cursor_on_cover = false;
    cursor_on_label = false;

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
        QImage scaled_cd_case =
            cd_case.scaled(vertical ? QSize(qMin(rect().height() - padding * 2, cover_size_max), qMin(rect().height() - padding * 2, cover_size_max))
                                    : QSize(qMin(rect().width() - padding * 2, cover_size_max), qMin(rect().width() - padding * 2, cover_size_max)),
                           Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);

        int xOffsetCover = padding;
        int yOffsetCover = padding;

        int x = xOffsetCover;
        int y = yOffsetCover;

        // QImage scaled_cover = cover.scaled(cover_size, cover_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        cover_rect = QRect(x, y, scaled_cd_case.width(), scaled_cd_case.height());
        painter.drawImage(QPoint(x, y), scaled_cd_case);

        QFont artistFont;
        artistFont.setBold(true);
        if (artistFont.pixelSize() == -1) {
            artistFont.setPointSizeF(artistFont.pointSizeF() * 1.2);
        } else {
            artistFont.setPixelSize(artistFont.pixelSize() * 1.2);
        }

        QFont titleFont;
        titleFont.setItalic(true);
        if (titleFont.pixelSize() == -1) {
            titleFont.setPointSizeF(titleFont.pointSizeF() * 1.6);
        } else {
            titleFont.setPixelSize(titleFont.pixelSize() * 1.6);
        }

        QFont cdNumberFont;

        painter.setRenderHint(QPainter::Antialiasing);

        y += scaled_cd_case.height() + padding * 2;

        int xOffsetLabel = x;
        int yOffsetLabel = y;

        // Künstlername
        painter.setFont(artistFont);
        QFontMetrics fm1(artistFont);
        painter.drawText(x, y, fm1.elidedText(cdda_model->artist(), Qt::ElideRight, width() - padding));
        y += fm1.height() + 5;

        painter.setFont(titleFont);
        QFontMetrics fm2(titleFont);
        QString fullTitle = cdda_model->album();
        if (!cdda_model->year().isEmpty()) {
            fullTitle += QString(" (%1)").arg(cdda_model->year());
        }
        painter.drawText(x, y, fm2.elidedText(fullTitle, Qt::ElideRight, width() - padding));
        y += fm2.height();

        // flags of the disc as small badges in one row
        QList<QPair<QString, QBrush>> badges;
        if (m_hdcd.value_or(false))
            badges.append({QStringLiteral("HDCD"), palette().accent()});
        if (!cdda_model->cdInfo().preEmphasisTracks().isEmpty())
            badges.append({i18nc("badge in the disc header", "PRE-EMPHASIS"), QColor(0x8e, 0x44, 0xad)}); // purple
        if (m_cdg.value_or(false))
            badges.append({QStringLiteral("CD+G"), QColor(Qt::black)});
        const Audex::CDInfo::Medium medium = cdda_model->cdInfo().medium();
        if (medium == Audex::CDInfo::Medium::CdR || medium == Audex::CDInfo::Medium::CdRw)
            badges.append({medium == Audex::CDInfo::Medium::CdRw ? QStringLiteral("CD-RW") : QStringLiteral("CD-R"), QColor(0x7f, 0x8c, 0x8d)}); // grey
        if (!badges.isEmpty()) {
            y += padding;
            QFontMetrics fm3(cdNumberFont);
            int badgeX = x;
            painter.save();
            painter.setFont(cdNumberFont);
            for (const auto &[text, brush] : std::as_const(badges)) {
                const QRect badge(badgeX, y - fm3.ascent(), fm3.horizontalAdvance(text) + 12, fm3.height() + 4);
                painter.setBrush(brush);
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(badge, 4, 4);
                painter.setPen(palette().color(QPalette::BrightText));
                painter.drawText(badge, Qt::AlignCenter, text);
                badgeX = badge.right() + 6;
            }
            painter.restore();
            y += fm3.height() + 4;
        }

        if (cdda_model->discNumber() > 0) {
            painter.setFont(cdNumberFont);
            y += padding;
            painter.drawText(x, y, QString("[%1%2]").arg(i18n("CD Number: ")).arg(cdda_model->discNumber()));
        }

        label_rect = QRect(xOffsetLabel,
                           yOffsetLabel - fm1.height(),
                           qMax(fm1.horizontalAdvance(cdda_model->artist()), fm2.horizontalAdvance(fullTitle)),
                           y - yOffsetLabel);

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
        cover_painter.drawImage(125, 15, cdda_model->coverImage().scaled(QSize(1110, 1080), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    cover_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    cover_painter.drawImage(259, 0, latches);

    cover_painter.end();

    this->cd_case = cdcase_covered;
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
    const Audex::Metadata::CoverArt cover = cdda_model->cover();
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
    if (dialog.exec() == QDialog::Accepted)
        cdda_model->setCover(dialog.cover());
}

void CDDAHeaderWidget::show_cover_source()
{
    if (cdda_model && cdda_model->cover().originPage.isValid())
        QDesktopServices::openUrl(cdda_model->cover().originPage);
}

void CDDAHeaderWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (cover_rect.contains(event->pos())) {
        if (!cursor_on_cover) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_cover = true;
        }
    } else if (label_rect.contains(event->pos())) {
        if (!cursor_on_label) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_label = true;
        }
    } else {
        QApplication::restoreOverrideCursor();
        if (cursor_on_cover) {
            cursor_on_cover = false;
        } else if (cursor_on_label) {
            cursor_on_label = false;
        }
    }
}

void CDDAHeaderWidget::mousePressEvent(QMouseEvent *event)
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::mousePressEvent() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (cursor_on_cover) {
            QApplication::restoreOverrideCursor();
            if (cdda_model->cover().isNull()) {
                load();
            } else {
                view_cover();
            }
        }
        if (cursor_on_label) {
            QApplication::restoreOverrideCursor();
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
        }
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
    update();
}

void CDDAHeaderWidget::edit_data()
{
    if (!cdda_model) {
        qWarning() << "CDDAHeaderWidget::edit_data() called with null model pointers";
        Q_ASSERT(cdda_model);
        return;
    }

    QApplication::restoreOverrideCursor();
    cursor_on_label = false;

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
    if (cursor_on_cover) {
        QApplication::restoreOverrideCursor();
        cursor_on_cover = false;
        QMenu contextMenu(this);
        contextMenu.clear();
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

void CDDAHeaderWidget::setup_actions()
{
    action_collection = new KActionCollection(this);

    auto *chooseCoverAction = new QAction(this);
    chooseCoverAction->setText(i18n("Choose Cover..."));
    chooseCoverAction->setToolTip(i18n("Shows all images the Cover Art Archive has for this album (needs MusicBrainz metadata)."));
    action_collection->addAction("choose", chooseCoverAction);
    connect(chooseCoverAction, &QAction::triggered, this, &CDDAHeaderWidget::choose_cover);

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
