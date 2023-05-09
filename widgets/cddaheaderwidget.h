/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2015 Marco Nelles (audex@maniatek.com)
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CDDAHEADERWIDGET_H
#define CDDAHEADERWIDGET_H

#include <cmath>

#include <QApplication>
#include <QDesktopServices>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWidget>
#include <QX11Info>

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>

#include "preferences.h"

#include "dialogs/cddaheaderdatadialog.h"
#include "dialogs/coverbrowserdialog.h"
#include "dialogs/errordialog.h"
#include "models/cddamodel.h"
#include "utils/cachedimage.h"
#include "utils/coverfetcher.h"
#include "utils/tmpdir.h"

// fixed point defines
#define FP_BITS 10
#define FP_FACTOR (1 << FP_BITS)

enum FadeStyle { NoFade, FadeDown, FadeRight, FadeUp, FadeLeft };

enum MirrorStyle { NoMirror, MirrorOverX, MirrorOverY };

class CDDAHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CDDAHeaderWidget(CDDAModel *cddaModel, QWidget *parent = nullptr, const int coverSize = 128, const int padding = 20);
    ~CDDAHeaderWidget() override;
    QSize sizeHint() const override;
    void setCover(CachedImage *cover);

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enabled);

    void googleAuto();

Q_SIGNALS:
    void headerDataChanged();

    void coverUp();
    void coverDown();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void fetchCoverFinished(bool showDialog);

private Q_SLOTS:

    void update();
    void trigger_repaint();
    void cover_is_down();

    void google();
    void load();
    void save();
    void view_cover();
    void remove();

    void edit_data();
    void wikipedia();

    void set_cover(const QByteArray &cover);
    void fetch_first_cover();
    void fetch_cover_failed();
    void auto_fetch_cover_failed();

    void context_menu(const QPoint &point);

private:
    CDDAModel *cdda_model;
    KActionCollection *action_collection;
    int cover_size;
    int padding;

    quint16 i_cover_checksum;
    QImage i_cover;
    QImage i_cover_holding;

    QTimer timer;
    bool animation_up;
    bool animation_down;
    qreal scale_factor;
    bool scale_up;
    bool scale_down;
    qreal opacity_factor;
    bool fade_in;
    bool fade_out;

    QRect cover_rect;
    bool cursor_on_cover;

    QRect link1_rect;
    bool cursor_on_link1;

    QRect link2_rect;
    bool cursor_on_link2;

    void setup_actions();

    bool enabled;

    bool fetching_cover_in_progress;
    CoverBrowserDialog *cover_browser_dialog;

    TmpDir *tmp_dir;
};

#endif
