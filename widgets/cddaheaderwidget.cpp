/* AUDEX CDDA EXTRACTOR
 * Copyright (C) 2007-2013 Marco Nelles (audex@maniatek.com)
 * <http://kde.maniatek.com/audex>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cddaheaderwidget.h"
#include "utils/errordialog.h"

static QImage mirrorImage(const QImage &img, MirrorStyle mirrorStyle = MirrorOverX, FadeStyle fadeStyle = FadeDown) {

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This function is part of the Graphics Dojo project on Qt Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

  QImage tmpImage = img;
  if (mirrorStyle != NoMirror) tmpImage = tmpImage.mirrored(mirrorStyle == MirrorOverY, mirrorStyle == MirrorOverX);

  if (fadeStyle != NoFade) {

    QPoint p1, p2;

    if (fadeStyle == FadeDown)
      p2.setY(tmpImage.height());
    else if (fadeStyle == FadeUp)
      p1.setY(tmpImage.height());
    else if (fadeStyle == FadeRight)
      p2.setX(tmpImage.width());
    else if (fadeStyle == FadeLeft)
      p1.setX(tmpImage.width());

    QLinearGradient gradient(p1, p2);
    gradient.setColorAt(0, Qt::white);
    gradient.setColorAt(0.2, QColor(0, 0, 0, 20));
    gradient.setColorAt(1, Qt::transparent);

    QPainter p(&tmpImage);
    p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    p.fillRect(0, 0, tmpImage.width(), tmpImage.height(), gradient);
    p.end();

  }

  return tmpImage;

}

static QImage fadeImage(const QImage &img, float val, const QColor &color) {

  /* This function is part of the KDE libraries
  Copyright (C) 1998, 1999, 2001, 2002 Daniel M. Duley <mosfet@interaccess.com>
  (C) 1998, 1999 Christian Tibirna <ctibirna@total.net>
  (C) 1998, 1999 Dirk Mueller <mueller@kde.org> */

  QImage tmpImage = img;

  if (tmpImage.width() == 0 || tmpImage.height() == 0) return tmpImage;

  // We don't handle bitmaps
  if (tmpImage.depth() == 1) return tmpImage;

  unsigned char tbl[256];
  for (int i = 0; i < 256; ++i) tbl[i] = (int)(val * i + 0.5);

  int red = color.red();
  int green = color.green();
  int blue = color.blue();

  QRgb col;
  int r, g, b, cr, cg, cb;

  if (tmpImage.depth() <= 8) {
  // pseudo color
    for (int i=0; i<tmpImage.numColors(); i++) {
      col = tmpImage.color(i);
      cr = qRed(col); cg = qGreen(col); cb = qBlue(col);
      if (cr > red)
        r = cr - tbl[cr-red];
      else
        r = cr + tbl[red-cr];
      if (cg > green)
        g = cg - tbl[cg-green];
      else
        g = cg + tbl[green-cg];
      if (cb > blue)
        b = cb - tbl[cb-blue];
      else
        b = cb + tbl[blue-cb];
        tmpImage.setColor(i, qRgba(r, g, b, qAlpha(col)));

    }

  } else {

    // truecolor
    for (int y=0; y<tmpImage.height(); ++y) {
      QRgb *data = (QRgb*)tmpImage.scanLine(y);
      for (int x=0; x<tmpImage.width(); ++x) {
        col = *data;
        cr = qRed(col); cg = qGreen(col); cb = qBlue(col);
        if (cr > red)
          r = cr - tbl[cr-red];
        else
          r = cr + tbl[red-cr];
        if (cg > green)
          g = cg - tbl[cg-green];
        else
          g = cg + tbl[green-cg];
        if (cb > blue)
          b = cb - tbl[cb-blue];
        else
          b = cb + tbl[blue-cb];
        *data++ = qRgba(r, g, b, qAlpha(col));
      }
    }

  }

  return tmpImage;

}

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** The following five functions are part of the Graphics Dojo project
** on Trolltech Labs.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 or 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

static inline int sign(int x) {
  return x < 0 ? -1 : 1;
}

static inline int blend(int a, int b, int blendA, int blendB) {
  return blendA * a + blendB * b;
}

// bilinear blend
static inline int blend(unsigned int *s, int blendX, int blendY, int shift) {

  int src[] = {
    (s[0] >> shift) & 0xff,
    (s[1] >> shift) & 0xff,
    (s[2] >> shift) & 0xff,
    (s[3] >> shift) & 0xff
  };

  int da = blend(src[0], src[1], FP_FACTOR - blendX, blendX);
  int db = blend(src[2], src[3], FP_FACTOR - blendX, blendX);

  return blend(da, db, FP_FACTOR - blendY, blendY) / (FP_FACTOR * FP_FACTOR);
}

static void processScanlines(QImage &img, int sy, int ey, int sx, int ex) {
  int dx = sign(ex - sx);
  int dy = sign(ey - sy);

  QRgb *data = (QRgb *)img.bits();
  int width = img.width();
  int height = img.height();

  if (dy == 1 && sy >= height) return;
  if (dx == 1 && sx >= width) return;
  if (dy != 1 && sy < 0) return;
  if (dx != 1 && sx < 0) return;

  int actual_sy = (dy == 1) ? qMax(0, sy) : qMin(height - 1, sy);
  int actual_ey = (dy == 1) ? qMin(height, ey) : qMax(-1, ey);

  int actual_sx = (dx == 1) ? qMax(0, sx) : qMin(width - 1, sx);
  int actual_ex = (dx == 1) ? qMin(width, ex) : qMax(-1, ex);

  if (dy == 1 && actual_sy >= actual_ey) return;
  if (dy != 1 && actual_sy <= actual_ey) return;
  if (dx == 1 && actual_sx >= actual_ex) return;
  if (dx != 1 && actual_sx <= actual_ex) return;

  for (int y = actual_sy; y != actual_ey; y += dy) {
    QRgb *dst = data + y * width;

    int srcY = y * FP_FACTOR + (sy - y) * 8;

    int scanLine = srcY / FP_FACTOR;
    QRgb *srcA = scanLine >= 0 && scanLine < height ? data + scanLine * width : 0;

    ++scanLine;
    QRgb *srcB = scanLine >= 0 && scanLine < height ? data + scanLine * width : 0;

    if (!srcA && !srcB) {
      for (int x = sx; x != ex; x += dx) dst[x] = 0;
      continue;
    }

    int blendY = srcY % FP_FACTOR;

    for (int x = actual_sx; x != actual_ex; x += dx) {
      int srcX = x * FP_FACTOR + (sx - x) * 8;

      int da = srcX / FP_FACTOR;
      int db = da + 1;

      int blendX = srcX % FP_FACTOR;

      QRgb src[4] = { srcA ? srcA[da] : 0, srcA ? srcA[db] : 0, srcB ? srcB[da] : 0, srcB ? srcB[db] : 0 };

      int red = blend(src, blendX, blendY, 16);
      int green = blend(src, blendX, blendY, 8);
      int blue = blend(src, blendX, blendY, 0);

      red = blend(red, qRed(dst[x]), FP_FACTOR-32, 128)/FP_FACTOR;
      green = blend(green, qGreen(dst[x]), FP_FACTOR-32, 128)/FP_FACTOR;
      blue = blend(blue, qBlue(dst[x]), FP_FACTOR-32, 128)/FP_FACTOR;

      red = qMin(red, 255);
      green = qMin(green, 255);
      blue = qMin(blue, 255);

      dst[x] = qRgba(red, green, blue, qAlpha(dst[x]));
    }
  }

}

static void radialBlur(QImage &img, int cx, int cy) {

  int w = img.width();
  int h = img.height();

  processScanlines(img, cy-1, -1, cx-1, -1);
  processScanlines(img, cy-1, -1, cx, w);
  processScanlines(img, cy, h, cx-1, -1);
  processScanlines(img, cy, h, cx, w);

}

CDDAHeaderWidget ::CDDAHeaderWidget(CDDAModel *cddaModel, QWidget* parent, const int coverSize, const int padding) : QWidget(parent) {

  cdda_model = cddaModel;
  if (!cdda_model) {
    kDebug() << "CDDAModel is NULL!";
    return;
  }
  connect(cdda_model, SIGNAL(modelReset()), this, SLOT(update()));

  setup_actions();

  kDebug() << "coverSize:" << coverSize;
  this->cover_size = coverSize;
  
  this->i_cover_checksum = 1;

  this->padding = padding;

  animation_up = FALSE;
  animation_down = FALSE;
  scale_up = FALSE;
  scale_down = FALSE;
  fade_in = FALSE;
  fade_out = FALSE;
  scale_factor = 1.0;
  opacity_factor = 1.0;

  setMouseTracking(TRUE);
  cursor_on_cover = FALSE;
  cursor_on_link1 = FALSE;
  cursor_on_link2 = FALSE;

  connect(this, SIGNAL(coverDown()), this, SLOT(cover_is_down()));
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(context_menu(const QPoint&)));

  cover_browser_dialog = NULL;
  fetching_cover_in_progress = FALSE;
  
  setContextMenuPolicy(Qt::CustomContextMenu);

  timer.setInterval(40);
  connect(&timer, SIGNAL(timeout()), this, SLOT(trigger_repaint()));

  setMinimumSize(QSize(cover_size+(padding*2), (int)(cover_size*1.4)+(padding*2)));

  tmp_dir = new TmpDir("audex", "cover");
    
  update();

}

CDDAHeaderWidget::~CDDAHeaderWidget() {
  delete action_collection;
  delete tmp_dir;
}

QSize CDDAHeaderWidget::sizeHint() const {
  return QSize((cover_size*1.5)+(padding*2), (int)(cover_size*1.4)+(padding*2));
}

void CDDAHeaderWidget::setCover(CachedImage *cover) {
  
  if (cover) {
    i_cover_checksum = cover->checksum();
  } else {
    i_cover_checksum = 0;
  }
  
  if (this->i_cover.isNull()) {
    if (cover) {
      this->i_cover = cover->coverImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    } else {
      QImage image = QImage(KStandardDirs::locate("data", QString("audex/images/nocover.png")));
      this->i_cover = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    animation_up = TRUE;
    fade_in = TRUE;
    scale_factor = 0.7;
    opacity_factor = 0.0;
  } else {
    if (cover) {
      this->i_cover_holding = cover->coverImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    } else {
      QImage image = QImage(KStandardDirs::locate("data", QString("audex/images/nocover.png")));
      this->i_cover_holding = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    animation_down = TRUE;
    scale_down = TRUE;
    scale_factor = 1.0;
    opacity_factor = 1.0;
  }

  timer.start();

}

bool CDDAHeaderWidget::isEnabled() const {
  return enabled;
}

void CDDAHeaderWidget::setEnabled(bool enabled) {
  this->enabled = enabled;
  repaint();
}

void CDDAHeaderWidget::googleAuto() {

  if ((cdda_model->discInfo()==CDDAModel::DiscNoInfo) || (fetching_cover_in_progress)) return;

  QApplication::restoreOverrideCursor();
  cursor_on_cover = FALSE;
  fetching_cover_in_progress = TRUE;
  action_collection->action("fetch")->setEnabled(FALSE);

  cover_browser_dialog = new CoverBrowserDialog(this);
  
  connect(cover_browser_dialog, SIGNAL(allCoverThumbnailsFetched()), this, SLOT(fetch_first_cover()));
  QString artist = cdda_model->artist();
  QString title = cdda_model->title();
  int lastColonPos = title.lastIndexOf(':');
  while (lastColonPos > 0) {
    title = title.left(lastColonPos);
    lastColonPos = title.lastIndexOf(':');
  }
  cover_browser_dialog->fetchThumbnails(QString("%1 %2").arg(artist).arg(title), 1);

}

void CDDAHeaderWidget::paintEvent(QPaintEvent *event) {

  Q_UNUSED(event);

  QPainter p;

  p.begin(this);

  if (enabled) {
    bool vertical=rect().height()>rect().width() && rect().width()<((cover_size+padding)*2);
    int  xOffset=vertical ? padding : (padding*2)+cover_size,
         yOffset=vertical ? (padding*2)+cover_size+24 : padding;

    QImage scaled_cover = i_cover.scaled((int)(scale_factor*cover_size), (int)(scale_factor*cover_size), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QImage faded_cover = fadeImage(scaled_cover, 1-opacity_factor, palette().background().color());

    p.drawImage((cover_size/2-(scaled_cover.width()/2))+padding, (cover_size/2-(scaled_cover.height()/2))+padding, faded_cover);
    cover_rect = QRect(padding, padding, scaled_cover.width(), scaled_cover.height());

    QImage mirror_img = mirrorImage(faded_cover);
    //we shear it a along the x axis to produce the nice leaning to the side effect. (valid only for MirrorOverX)
    //mirror_img = mirror_img.transformed(QMatrix().shear(0.2, 0.0), Qt::SmoothTransformation);
    radialBlur(mirror_img, 50, 50);

    p.drawImage((cover_size/2-(scaled_cover.width()/2))+padding, (cover_size/2-(scaled_cover.height()/2))+padding+mirror_img.height()+5, mirror_img);

    p.setBrush(palette().text());

    QFont font(QApplication::font());
    int pixelSize(font.pixelSize()==-1 ? (font.pointSize()*QX11Info::appDpiX()+36)/72 : font.pixelSize()), width=rect().width()-(xOffset+1);
    font.setPixelSize((int)((((double)pixelSize)*1.5)+0.5));
    font.setBold(TRUE);
    p.setFont(font);
    yOffset+=p.fontMetrics().lineSpacing()*1.2;
    p.drawText(xOffset, yOffset, p.fontMetrics().elidedText(cdda_model->artist(), Qt::ElideRight, width));

    font.setPixelSize(pixelSize);
    font.setBold(TRUE);
    font.setItalic(TRUE);
    p.setFont(font);
    yOffset+=pixelSize;
    p.drawText(xOffset, yOffset, p.fontMetrics().elidedText(cdda_model->title(), Qt::ElideRight, width));

    yOffset+=p.fontMetrics().lineSpacing()*1.5;
    font.setBold(FALSE);
    font.setItalic(FALSE);
    p.setFont(font);

    QFontMetrics fm(font);
    QString      yearLabel(i18n("Released: ")),
                 genreLabel(i18n("Genre: ")),
                 cdNoLabel(i18n("CD Number: "));
    int          maxWidth(fm.width(yearLabel));

    if ((width=fm.width(genreLabel))>maxWidth) maxWidth = width;
    if (cdda_model->isMultiCD() && (width=fm.width(cdNoLabel))) maxWidth = width;

    width = rect().width()-(xOffset+1);

    if (!cdda_model->year().isEmpty()) {
      p.drawText(xOffset, yOffset, yearLabel);
      p.drawText(xOffset+maxWidth, yOffset, fm.elidedText(cdda_model->year(), Qt::ElideRight, width-maxWidth));
      yOffset += fm.lineSpacing();
    }
    if (!cdda_model->genre().isEmpty()) {
      p.drawText(xOffset, yOffset, genreLabel);
      p.drawText(xOffset+maxWidth, yOffset, fm.elidedText(cdda_model->genre(), Qt::ElideRight, width-maxWidth));
      yOffset += fm.lineSpacing();
    }
    if (cdda_model->isMultiCD()) {
      p.drawText(xOffset, yOffset, cdNoLabel);
      p.drawText(xOffset+maxWidth, yOffset, QString().setNum(cdda_model->cdNum()));
      yOffset += fm.lineSpacing();
    }

    font.setUnderline(TRUE);
    p.setFont(font);

    //links

    fm = QFontMetrics(font);
    QString link1 = i18n("Edit Data");
    QString link2 = i18n("Wikipedia");
    KColorScheme kcs(QPalette::Active);
    p.setPen(kcs.foreground(KColorScheme::LinkText).color());

    link1_rect = fm.boundingRect(link1);
    link2_rect = fm.boundingRect(link2);

    yOffset = vertical ? yOffset+fm.lineSpacing() : (yOffset>(padding+cover_size) ? yOffset : (padding+cover_size));
    p.drawText(xOffset, yOffset, link1);
    p.drawText(xOffset+(link1_rect.height()/2)+link1_rect.width(), yOffset, link2);
    link1_rect = QRect(xOffset, yOffset+link1_rect.y(), link1_rect.width(), link1_rect.height());
    link2_rect = QRect(xOffset+(link1_rect.height()/2)+link1_rect.width(), yOffset+link2_rect.y(), link2_rect.width(), link2_rect.height());
    
  } else { //disabled

    QFont font(QApplication::font());
    if (-1==font.pixelSize()) {
      font.setPointSizeF(font.pointSizeF()*1.5);
    } else {
      font.setPixelSize(font.pixelSize()*1.5);
    }
    font.setBold(TRUE);
    font.setItalic(TRUE);
    p.setFont(font);
    p.drawText(rect(), Qt::AlignCenter | Qt::AlignVCenter, i18n("No CD in drive"));

  }

  p.end();

}

void CDDAHeaderWidget::mouseMoveEvent(QMouseEvent *event) {

  if (cover_rect.contains(event->pos())) {

    if (!cursor_on_cover) { QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor)); cursor_on_cover = TRUE; }

  } else if (link1_rect.contains(event->pos())) {

    if (!cursor_on_link1) { QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor)); cursor_on_link1 = TRUE; }

  } else if (link2_rect.contains(event->pos())) {

    if (!cursor_on_link2) { QApplication::setOverrideCursor(QCursor(Qt::PointingHandCursor)); cursor_on_link2 = TRUE; }

  } else {

    if (cursor_on_cover) {
      QApplication::restoreOverrideCursor();
      cursor_on_cover = FALSE;
    } else if (cursor_on_link1) {
      QApplication::restoreOverrideCursor();
      cursor_on_link1 = FALSE;
    } else if (cursor_on_link2) {
      QApplication::restoreOverrideCursor();
      cursor_on_link2 = FALSE;
    }

  }

}

void CDDAHeaderWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    if ((cursor_on_cover) && (!fetching_cover_in_progress)) {
      if (cdda_model->isCoverEmpty()) {
	if (cdda_model->discInfo() == CDDAModel::DiscNoInfo) {
	  load();
	} else {
	  google();
	}
      } else {
	view_cover();
      }
    }
    if (cursor_on_link1) edit_data();
    if (cursor_on_link2) wikipedia();
  }
}

void CDDAHeaderWidget::update() {

  action_collection->action("fetch")->setEnabled(cdda_model->discInfo() != CDDAModel::DiscNoInfo);

  bool activate = FALSE;
  if (cdda_model->isCoverEmpty()) {
    if (i_cover_checksum) setCover(NULL);
  } else {
    kDebug() << "current cover checksum:" << i_cover_checksum;
    kDebug() << "new cover checksum:" << cdda_model->coverChecksum();
    if (i_cover_checksum != cdda_model->coverChecksum()) setCover(cdda_model->cover());
    activate = TRUE;
  }

  action_collection->action("save")->setEnabled(activate);
  action_collection->action("view")->setEnabled(activate);
  action_collection->action("remove")->setEnabled(activate);
  
  repaint();

}

void CDDAHeaderWidget::trigger_repaint() {

  if (animation_down) {

    if (scale_down) {
      scale_factor -= 0.08;
      if (qFuzzyCompare(scale_factor, .7) || scale_factor < .7) {
        scale_down = FALSE;
	fade_out = TRUE;
      }
    } else if (fade_out) {
      opacity_factor -= 0.16;
      if (qFuzzyCompare(opacity_factor, 0) || opacity_factor < 0) {
        opacity_factor = 0;
	fade_out = FALSE;
	animation_down = FALSE;
	timer.stop();
	emit coverDown();
      }
    }

  } else if (animation_up) {

    if (fade_in) {
      opacity_factor += 0.16;
      if (qFuzzyCompare(opacity_factor, 1) || opacity_factor > 1) {
        opacity_factor = 1;
	fade_in = FALSE;
	scale_up = TRUE;
      }
    } else if (scale_up) {
      scale_factor += 0.08;
      if (qFuzzyCompare(scale_factor, 1) || scale_factor > 1) {
        scale_up = FALSE;
	animation_up = FALSE;
	timer.stop();
	emit coverUp();
      }
    }

  }

  repaint();

}

void CDDAHeaderWidget::cover_is_down() {
  this->i_cover = i_cover_holding;
  animation_up = TRUE;
  fade_in = TRUE;
  scale_factor = .7;
  opacity_factor = 0.0;
  timer.start();
}

void CDDAHeaderWidget::google() {

  if ((cdda_model->discInfo() == CDDAModel::DiscNoInfo) || (fetching_cover_in_progress)) return;

  QApplication::restoreOverrideCursor();
  cursor_on_cover = FALSE;
  fetching_cover_in_progress = TRUE;
  action_collection->action("fetch")->setEnabled(FALSE);

  cover_browser_dialog = new CoverBrowserDialog(this);
  
  connect(cover_browser_dialog, SIGNAL(coverFetched(const QByteArray&)), this, SLOT(set_cover(const QByteArray&)));
  QString artist = cdda_model->artist();
  QString title = cdda_model->title();
  int lastColonPos = title.lastIndexOf(':');
  while (lastColonPos > 0) {
    title = title.left(lastColonPos);
    lastColonPos = title.lastIndexOf(':');
  }
  cover_browser_dialog->fetchThumbnails(QString("%1 %2").arg(artist).arg(title));

  if (cover_browser_dialog->exec() != QDialog::Accepted) {
    fetching_cover_in_progress = FALSE;
    delete cover_browser_dialog;
    cover_browser_dialog = NULL;
    action_collection->action("fetch")->setEnabled(TRUE);
  }

}

void CDDAHeaderWidget::load() {
  kDebug() << "Supported cover image file MIME types:" << cdda_model->coverSupportedMimeTypeList();
  QString filename = KFileDialog::getOpenFileName(KUrl(QDir::homePath()), cdda_model->coverSupportedMimeTypeList(), this, i18n("Load Cover"));
  if (!filename.isEmpty()) {
    if (!cdda_model->setCover(filename)) {
      ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
    }
  }
}

void CDDAHeaderWidget::save() {
  QString filename = KFileDialog::getSaveFileName(KUrl(QDir::homePath()+"/"+cdda_model->title()+".jpg"), cdda_model->coverSupportedMimeTypeList(), this, i18n("Save Cover"));
  if (!filename.isEmpty()) {
    if (!cdda_model->saveCoverToFile(filename)) {
      ErrorDialog::show(this, cdda_model->lastError().message(), cdda_model->lastError().details());
    }
  }
}

void CDDAHeaderWidget::view_cover() {
  
  QString tmp_path = tmp_dir->tmpPath();
  if (tmp_dir->error()) {
    QStringList dirs = KGlobal::dirs()->resourceDirs("tmp");
    tmp_path = dirs.size()?dirs[0]:"/var/tmp/";
    if (tmp_path.right(1) != "/") tmp_path += "/";
    kDebug() << "Temporary folder in use:" << tmp_path;
  }
  
  QString filename = tmp_path+QString("%1.jpeg").arg(cdda_model->coverChecksum());
  cdda_model->saveCoverToFile(filename);
  
  QDesktopServices::openUrl(KUrl(filename));

}

void CDDAHeaderWidget::remove() {
  cdda_model->clearCover();
  update();
}

void CDDAHeaderWidget::edit_data() {

  QApplication::restoreOverrideCursor();
  cursor_on_link1 = FALSE;

  CDDAHeaderDataDialog *dialog = new CDDAHeaderDataDialog(cdda_model, this);

  if (dialog->exec() != QDialog::Accepted) { delete dialog; return; }
  delete dialog;
  update();
  emit headerDataChanged();

}

void CDDAHeaderWidget::wikipedia() {

  int locale = Preferences::wikipediaLocale();
  QString l;
  if (locale==0) l = "en";
  if (locale==1) l = "de";
  if (locale==2) l = "fr";
  if (locale==3) l = "pl";
  if (locale==4) l = "ja";
  if (locale==5) l = "it";
  if (locale==6) l = "nl";
  if (locale==7) l = "es";
  if (locale==8) l = "pt";
  if (locale==9) l = "sv";

  if (l.isEmpty()) l = "en";

  QDesktopServices::openUrl(QUrl(QString("http://%1.wikipedia.org/wiki/").arg(l) + KUrl::toPercentEncoding(cdda_model->artist())));

}

void CDDAHeaderWidget::set_cover(const QByteArray& cover) {
  if (!cover.isEmpty()) cdda_model->setCover(cover);
  fetching_cover_in_progress = FALSE;
  action_collection->action("fetch")->setEnabled(TRUE);
  if (cover_browser_dialog) {
    delete cover_browser_dialog;
    cover_browser_dialog = NULL;
  }
  if (!cover.isEmpty()) update();
}

void CDDAHeaderWidget::fetch_first_cover() {
  if (cover_browser_dialog) {
    if (cover_browser_dialog->count() == 0) {
      kDebug() << "no cover found";
      ErrorDialog::show(this, i18n("No cover found."), i18n("Check your artist name and title. Otherwise you can load a custom cover from an image file."));
      delete cover_browser_dialog;
      cover_browser_dialog = NULL;
      fetching_cover_in_progress = FALSE;
      action_collection->action("fetch")->setEnabled(TRUE);
    } else {
      connect(cover_browser_dialog, SIGNAL(coverFetched(const QByteArray&)), this, SLOT(set_cover(const QByteArray&)));
      cover_browser_dialog->startFetchCover(0);
    }
  }
}

void CDDAHeaderWidget::context_menu(const QPoint& point) {
  kDebug() << "context menu requested at point" << point;
  if ((cursor_on_cover) && (!fetching_cover_in_progress)) {
    QApplication::restoreOverrideCursor();
    cursor_on_cover = FALSE;
    KMenu contextMenu(this);
    QMouseEvent *mevent = new QMouseEvent(QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    contextMenu.clear();
    contextMenu.addAction(action_collection->action("fetch"));
    contextMenu.addAction(action_collection->action("load"));
    contextMenu.addAction(action_collection->action("save"));
    contextMenu.addSeparator();
    contextMenu.addAction(action_collection->action("view"));
    contextMenu.addSeparator();
    contextMenu.addAction(action_collection->action("remove"));
    contextMenu.exec(mevent->globalPos());
  }
}

void CDDAHeaderWidget::setup_actions() {

  action_collection = new KActionCollection(this);

  KAction* fetchCoverAction = new KAction(this);
  fetchCoverAction->setText(i18n("Fetch cover from Google..."));
  action_collection->addAction("fetch", fetchCoverAction);
  connect(fetchCoverAction, SIGNAL(triggered(bool)), this, SLOT(google()));

  KAction* loadCoverAction = new KAction(this);
  loadCoverAction->setText(i18n("Set Custom Cover..."));
  action_collection->addAction("load", loadCoverAction);
  connect(loadCoverAction, SIGNAL(triggered(bool)), this, SLOT(load()));

  KAction* saveCoverAction = new KAction(this);
  saveCoverAction->setText(i18n("Save Cover To File..."));
  action_collection->addAction("save", saveCoverAction);
  connect(saveCoverAction, SIGNAL(triggered(bool)), this, SLOT(save()));

  KAction* viewCoverAction = new KAction(this);
  viewCoverAction->setText(i18n("Show Full Size Cover..."));
  action_collection->addAction("view", viewCoverAction);
  connect(viewCoverAction, SIGNAL(triggered(bool)), this, SLOT(view_cover()));

  KAction* removeCoverAction = new KAction(this);
  removeCoverAction->setText(i18n("Remove Cover"));
  action_collection->addAction("remove", removeCoverAction);
  connect(removeCoverAction, SIGNAL(triggered(bool)), this, SLOT(remove()));

}
