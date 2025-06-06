/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cddaheaderwidget.h"

#include <QImageReader>

#include "dialogs/cddaheaderdatadialog.h"

namespace Audex
{

CDDAHeaderWidget ::CDDAHeaderWidget(QWidget *parent, const int cover_size_min, const int cover_size_max, const int padding)
    : QWidget(parent)
{
    setup_actions();

    this->cover_size_min = cover_size_min;
    this->cover_size_max = cover_size_max;
    this->padding = padding;

    setMouseTracking(true);
    cursor_on_cover = false;
    cursor_on_label = false;

    QObject::connect(this, &CDDAHeaderWidget::customContextMenuRequested, this, &CDDAHeaderWidget::context_menu);

    setContextMenuPolicy(Qt::CustomContextMenu);

    setMinimumSize(QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2)));

    update();
}

CDDAHeaderWidget::~CDDAHeaderWidget()
{
}

QSize CDDAHeaderWidget::sizeHint() const
{
    return QSize(cover_size_min + (padding * 2), cover_size_min + (padding * 2));
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

void CDDAHeaderWidget::editData()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    QApplication::restoreOverrideCursor();
    cursor_on_label = false;

    QPointer<CDDAHeaderDataDialog> dialog = new CDDAHeaderDataDialog(metadata, toc, this);
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }

    if (metadata != dialog->metadata()) {
        metadata = dialog->metadata();
        update();
        Q_EMIT metadataChanged(metadata);
    }
}

void CDDAHeaderWidget::setMetadata(const Metadata::Metadata &metadata)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    this->metadata = metadata;
    update();
}

void CDDAHeaderWidget::setCDDA(const CDDA &cdda)
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    this->metadata = cdda.metadata();
    this->toc = cdda.toc();
    update();
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

        // artist name
        painter.setFont(artistFont);
        QFontMetrics fm1(artistFont);
        painter.drawText(x, y, fm1.elidedText(metadata.get(Metadata::Type::Artist).toString(), Qt::ElideRight, width() - padding));
        y += fm1.height() + 5;

        painter.setFont(titleFont);
        QFontMetrics fm2(titleFont);
        QString fullTitle = metadata.get(Metadata::Type::Album).toString();
        if (metadata.contains(Metadata::Type::Year)) {
            fullTitle += QStringLiteral(" (%1)").arg(metadata.get(Metadata::Type::Year).toString());
        }
        painter.drawText(x, y, fm2.elidedText(fullTitle, Qt::ElideRight, width() - padding));
        y += fm2.height();

        if (metadata.isMultiDisc()) {
            painter.setFont(cdNumberFont);
            y += padding;
            painter.drawText(x, y, QStringLiteral("[%1%2]").arg(i18n("CD Number: ")).arg(metadata.get(Metadata::Type::DiscNumber).toInt()));
        }

        label_rect = QRect(xOffsetLabel,
                           yOffsetLabel - fm1.height(),
                           qMax(fm1.horizontalAdvance(metadata.get(Metadata::Type::Artist).toString()), fm2.horizontalAdvance(fullTitle)),
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
    if (!metadata.cover().isNull())
        cover_painter.drawImage(125, 15, metadata.cover().scaled(QSize(1110, 1080), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

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
    if (event->button() == Qt::LeftButton) {
        if (cursor_on_cover) {
            QApplication::restoreOverrideCursor();
            if (metadata.cover().isNull()) {
                load();
            } else {
                view_cover();
            }
        }
        if (cursor_on_label) {
            QApplication::restoreOverrideCursor();
            editData();
        }
    }
}

void CDDAHeaderWidget::update()
{
    qDebug() << "DEBUG:" << __FILE__ << __PRETTY_FUNCTION__;

    bool activate = false;
    if (!metadata.cover().isNull()) {
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
    QStringList filters;
    QList<QByteArray> formats = QImageReader::supportedImageFormats();

    QStringList extensions;
    for (const QByteArray &format : formats) {
        extensions << "*." + QString::fromLatin1(format).toLower();
    }

    QString filter = QString("Supported Images (%1)").arg(extensions.join(' '));

    QString filename = QFileDialog::getOpenFileName(this, i18n("Load Cover"), QDir::homePath(), filter);
    if (!filename.isEmpty()) {
        if (!metadata.loadCoverFromFile(filename)) {
            // ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
        } else {
            update();
            Q_EMIT metadataChanged(metadata);
        }
    }
}

void CDDAHeaderWidget::save()
{
    QString filename = QFileDialog::getSaveFileName(this, i18n("Save Cover"), QDir::homePath() + '/' + metadata.get(Metadata::Type::Title).toString() + ".jpg");
    if (!filename.isEmpty()) {
        if (!metadata.saveCoverToFile(filename)) {
            // ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
        }
    }
}

void CDDAHeaderWidget::view_cover()
{
    QString filename = tmp_dir.path() + QString("%1.jpeg").arg(metadata.cover().cacheKey(), 8, 16);
    metadata.saveCoverToFile(filename);

    qDebug() << "Open" << filename;

    QDesktopServices::openUrl(QUrl(filename));
}

void CDDAHeaderWidget::remove()
{
    metadata.setCover(QImage());
    update();
    Q_EMIT metadataChanged(metadata);
}

void CDDAHeaderWidget::set_cover(const QByteArray &cover)
{
    if (!cover.isEmpty())
        metadata.setCover(QImage(cover));
    update();
}

void CDDAHeaderWidget::context_menu(const QPoint &point)
{
    qDebug() << "context menu requested at point" << point;
    if (cursor_on_cover) {
        QApplication::restoreOverrideCursor();
        cursor_on_cover = false;
        QMenu contextMenu(this);
        contextMenu.clear();
        // contextMenu.addAction(action_collection->action("fetch"));
        contextMenu.addAction(action_collection->action("load"));
        contextMenu.addAction(action_collection->action("save"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("view"));
        contextMenu.addSeparator();
        contextMenu.addAction(action_collection->action("remove"));
        contextMenu.exec(mapToGlobal(point));
    }
}

void CDDAHeaderWidget::setup_actions()
{
    action_collection = new KActionCollection(this);

    auto loadCoverAction = new QAction(this);
    loadCoverAction->setText(i18n("Set Custom Cover..."));
    action_collection->addAction("load", loadCoverAction);
    connect(loadCoverAction, SIGNAL(triggered(bool)), this, SLOT(load()));

    auto saveCoverAction = new QAction(this);
    saveCoverAction->setText(i18n("Save Cover To File..."));
    action_collection->addAction("save", saveCoverAction);
    connect(saveCoverAction, SIGNAL(triggered(bool)), this, SLOT(save()));

    auto viewCoverAction = new QAction(this);
    viewCoverAction->setText(i18n("Show Full Size Cover..."));
    action_collection->addAction("view", viewCoverAction);
    connect(viewCoverAction, SIGNAL(triggered(bool)), this, SLOT(view_cover()));

    auto removeCoverAction = new QAction(this);
    removeCoverAction->setText(i18n("Remove Cover"));
    action_collection->addAction("remove", removeCoverAction);
    connect(removeCoverAction, SIGNAL(triggered(bool)), this, SLOT(remove()));
}

}
