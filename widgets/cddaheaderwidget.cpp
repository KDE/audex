/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderwidget.h"

#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <qnamespace.h>

CDDAHeaderWidget ::CDDAHeaderWidget(CDDAModel *cddaModel, QWidget *parent, const int cover_size_min, const int cover_size_max, const int padding)
    : QWidget(parent)
{
    cdda_model = cddaModel;
    if (!cdda_model) {
        qDebug() << "CDDAModel is NULL!";
        return;
    }
    connect(cdda_model, SIGNAL(modelReset()), this, SLOT(update()));

    setup_actions();

    this->cover_size_min = cover_size_min;
    this->cover_size_max = cover_size_max;
    this->padding = padding;

    this->cover_checksum = 1;

    setMouseTracking(true);
    cursor_on_cover = false;
    cursor_on_artist_label = false;
    cursor_on_title_label = false;
    cursor_on_year_label = false;

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(context_menu(const QPoint &)));

    setContextMenuPolicy(Qt::CustomContextMenu);

    setMinimumSize(QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2)));

    tmp_dir = new TmpDir("audex", "cover");

    update();
}

CDDAHeaderWidget::~CDDAHeaderWidget()
{
    delete action_collection;
    delete tmp_dir;
}

QSize CDDAHeaderWidget::sizeHint() const
{
    return QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2));
}

void CDDAHeaderWidget::setCover(CachedImage *cover)
{
    if (cover) {
        cover_checksum = cover->checksum();
        this->cover = cover->coverImage();
    } else {
        cover_checksum = 0;
        this->cover = QImage();
    }
    construct_cd_case();
}

bool CDDAHeaderWidget::isEnabled() const
{
    return enabled;
}

void CDDAHeaderWidget::setEnabled(bool enabled)
{
    this->enabled = enabled;
    repaint();
}

void CDDAHeaderWidget::paintEvent(QPaintEvent *event)
{
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

        int xOffset = padding;
        int yOffset = padding;

        // QImage scaled_cover = cover.scaled(cover_size, cover_size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        cover_rect = QRect(xOffset, yOffset, scaled_cd_case.width(), scaled_cd_case.height());
        painter.drawImage(QPoint(xOffset, yOffset), scaled_cd_case);

        if (vertical) {
            xOffset += scaled_cd_case.width() + padding;
        } else {
            yOffset += scaled_cd_case.height();
        }

        painter.setBrush(palette().text());

        QFont font(QApplication::font());

        int pixelSize = font.pixelSize() == -1 ? (font.pointSize() * QX11Info::appDpiX() + 36) / 72 : font.pixelSize();
        int width = rect().width() - (xOffset + 1);
        font.setPixelSize((int)((((double)pixelSize) * 2) + 0.5));
        font.setBold(true);
        font.setUnderline(cursor_on_artist_label || cursor_on_title_label || cursor_on_year_label);
        painter.setFont(font);
        artist_label_rect = QRect(QPoint(xOffset, yOffset + 10), painter.fontMetrics().boundingRect(cdda_model->artist()).size());
        yOffset += painter.fontMetrics().lineSpacing() + 10;
        painter.drawText(xOffset, yOffset, painter.fontMetrics().elidedText(cdda_model->artist(), Qt::ElideRight, width));

        font.setPixelSize((int)((((double)pixelSize) * 1.6) + 0.5));
        font.setBold(true);
        font.setItalic(true);
        painter.setFont(font);
        title_label_rect = QRect(QPoint(xOffset, yOffset + 10), painter.fontMetrics().boundingRect(cdda_model->title()).size());
        yOffset += (int)((((double)pixelSize) * 1.6) + 0.5) + 10;
        painter.drawText(xOffset, yOffset, painter.fontMetrics().elidedText(cdda_model->title(), Qt::ElideRight, width));

        if (!cdda_model->year().isEmpty()) {
            font.setItalic(false);
            font.setUnderline(false);
            painter.setFont(font);
            painter.drawText(xOffset + title_label_rect.width(), yOffset, QString(" (%1)").arg(cdda_model->year()));
            year_label_rect = QRect(QPoint(xOffset + title_label_rect.width(), title_label_rect.y()),
                                    painter.fontMetrics().boundingRect(QString(" (%1)").arg(cdda_model->year())).size());
        }

        if (cdda_model->cdNum() > 0) {
            font.setItalic(false);
            font.setUnderline(false);
            font.setPixelSize((int)((((double)pixelSize)) + 0.5));
            painter.setFont(font);
            yOffset += painter.fontMetrics().lineSpacing() + 10;
            painter.drawText(xOffset, yOffset, QString("[%1%2]").arg(i18n("CD Number: ")).arg(cdda_model->cdNum()));
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

void CDDAHeaderWidget::construct_cd_case()
{
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
    if (cover_checksum != 0)
        cover_painter.drawImage(125, 15, this->cover.scaled(QSize(1110, 1080), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    cover_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    cover_painter.drawImage(259, 0, latches);

    cover_painter.end();

    this->cd_case = cdcase_covered;
}

void CDDAHeaderWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (cover_rect.contains(event->pos())) {
        if (!cursor_on_cover) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_cover = true;
        }
    } else if (artist_label_rect.contains(event->pos())) {
        if (!cursor_on_artist_label) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_artist_label = true;
            repaint();
        }
    } else if (title_label_rect.contains(event->pos())) {
        if (!cursor_on_title_label) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_title_label = true;
            repaint();
        }
    } else if (year_label_rect.contains(event->pos())) {
        if (!cursor_on_year_label) {
            QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor));
            cursor_on_year_label = true;
            repaint();
        }
    } else {
        QApplication::restoreOverrideCursor();
        if (cursor_on_cover) {
            cursor_on_cover = false;
        } else if (cursor_on_artist_label) {
            cursor_on_artist_label = false;
            repaint();
        } else if (cursor_on_title_label) {
            cursor_on_title_label = false;
            repaint();
        } else if (cursor_on_year_label) {
            cursor_on_year_label = false;
            repaint();
        }
    }
}

void CDDAHeaderWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (cursor_on_cover) {
            QApplication::restoreOverrideCursor();
            if (cdda_model->isCoverEmpty()) {
                load();
            } else {
                view_cover();
            }
        }
        if (cursor_on_artist_label || cursor_on_title_label || cursor_on_year_label) {
            QApplication::restoreOverrideCursor();
            edit_data();
        }
    }
}

void CDDAHeaderWidget::update()
{
    // action_collection->action("fetch")->setEnabled(!cdda_model->empty());

    bool activate = false;
    if (cdda_model->isCoverEmpty()) {
        if (cover_checksum)
            setCover(nullptr);
    } else {
        qDebug() << "current cover checksum:" << cover_checksum;
        qDebug() << "new cover checksum:" << cdda_model->coverChecksum();
        if (cover_checksum != cdda_model->coverChecksum())
            setCover(cdda_model->cover());
        activate = true;
    }

    action_collection->action("save")->setEnabled(activate);
    action_collection->action("view")->setEnabled(activate);
    action_collection->action("remove")->setEnabled(activate);

    construct_cd_case();
    repaint();
}

void CDDAHeaderWidget::load()
{
    qDebug() << "Supported cover image file MIME types:" << cdda_model->coverSupportedMimeTypeList();
    QString filename = QFileDialog::getOpenFileName(this, i18n("Load Cover"), QDir::homePath(), cdda_model->coverSupportedMimeTypeList());
    if (!filename.isEmpty()) {
        if (!cdda_model->setCover(filename)) {
            ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
        }
    }
}

void CDDAHeaderWidget::save()
{
    QString filename = QFileDialog::getSaveFileName(this, i18n("Save Cover"), QDir::homePath() + '/' + cdda_model->title() + ".jpg");
    if (!filename.isEmpty()) {
        if (!cdda_model->saveCoverToFile(filename)) {
            ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
        }
    }
}

void CDDAHeaderWidget::view_cover()
{
    QString tmp_path = tmp_dir->tmpPath();
    if (tmp_dir->error()) {
        QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
        tmp_path = dirs.size() ? dirs[0] : "/var/tmp/";
        if (tmp_path.right(1) != "/")
            tmp_path += "/";
        qDebug() << "Temporary folder in use:" << tmp_path;
    }

    QString filename = tmp_path + QString("%1.jpeg").arg(cdda_model->coverChecksum());
    cdda_model->saveCoverToFile(filename);

    QDesktopServices::openUrl(QUrl(filename));
}

void CDDAHeaderWidget::remove()
{
    cdda_model->clearCover();
    update();
}

void CDDAHeaderWidget::edit_data()
{
    QApplication::restoreOverrideCursor();
    cursor_on_artist_label = false;
    cursor_on_title_label = false;
    cursor_on_year_label = false;

    auto *dialog = new CDDAHeaderDataDialog(cdda_model, this);

    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        return;
    }
    delete dialog;
    update();
    Q_EMIT headerDataChanged();
}

void CDDAHeaderWidget::set_cover(const QByteArray &cover)
{
    if (!cover.isEmpty())
        cdda_model->setCover(cover);
    action_collection->action("fetch")->setEnabled(true);
    update();
}

void CDDAHeaderWidget::context_menu(const QPoint &point)
{
    qDebug() << "context menu requested at point" << point;
    if (cursor_on_cover) {
        QApplication::restoreOverrideCursor();
        cursor_on_cover = false;
        QMenu contextMenu(this);
        auto *mevent = new QMouseEvent(QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        contextMenu.clear();
        // contextMenu.addAction(action_collection->action("fetch"));
        contextMenu.addAction(action_collection->action("load"));
        contextMenu.addAction(action_collection->action("save"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("view"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("remove"));
        contextMenu.exec(mevent->globalPos());
    }
}

void CDDAHeaderWidget::setup_actions()
{
    action_collection = new KActionCollection(this);

    // auto *fetchCoverAction = new QAction(this);
    // fetchCoverAction->setText(i18n("Fetch cover from Google..."));
    // action_collection->addAction("fetch", fetchCoverAction);
    // connect(fetchCoverAction, SIGNAL(triggered(bool)), this, SLOT(google()));

    auto *loadCoverAction = new QAction(this);
    loadCoverAction->setText(i18n("Set Custom Cover..."));
    action_collection->addAction("load", loadCoverAction);
    connect(loadCoverAction, SIGNAL(triggered(bool)), this, SLOT(load()));

    auto *saveCoverAction = new QAction(this);
    saveCoverAction->setText(i18n("Save Cover To File..."));
    action_collection->addAction("save", saveCoverAction);
    connect(saveCoverAction, SIGNAL(triggered(bool)), this, SLOT(save()));

    auto *viewCoverAction = new QAction(this);
    viewCoverAction->setText(i18n("Show Full Size Cover..."));
    action_collection->addAction("view", viewCoverAction);
    connect(viewCoverAction, SIGNAL(triggered(bool)), this, SLOT(view_cover()));

    auto *removeCoverAction = new QAction(this);
    removeCoverAction->setText(i18n("Remove Cover"));
    action_collection->addAction("remove", removeCoverAction);
    connect(removeCoverAction, SIGNAL(triggered(bool)), this, SLOT(remove()));
}
