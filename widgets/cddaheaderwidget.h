/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007 Marco Nelles
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
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>
#include <QX11Info>

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>

#include "preferences.h"

#include "dialogs/cddaheaderdatadialog.h"
#include "dialogs/errordialog.h"
#include "models/cddamodel.h"

// fixed point defines
#define FP_BITS 10
#define FP_FACTOR (1 << FP_BITS)

enum FadeStyle { NoFade, FadeDown, FadeRight, FadeUp, FadeLeft };

enum MirrorStyle { NoMirror, MirrorOverX, MirrorOverY };

class CDDAHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CDDAHeaderWidget(CDDAModel *cddaModel,
                              QWidget *parent = nullptr,
                              const int cover_size_min = 200,
                              const int cover_size_max = 400,
                              const int padding = 20);
    ~CDDAHeaderWidget() override;
    QSize sizeHint() const override;
    // void setCover(const QImage &cover);

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enabled);
    void edit_data();

Q_SIGNALS:
    void headerDataChanged();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void fetchCoverFinished(bool showDialog);

private Q_SLOTS:

    void update();

    void load();
    void save();
    void view_cover();
    void remove();

    void set_cover(const QByteArray &cover);

    void context_menu(const QPoint &point);

private:
    CDDAModel *cdda_model;
    KActionCollection *action_collection;
    int cover_size_min;
    int cover_size_max;
    int padding;

    QImage cd_case;
    void construct_cd_case();

    QRect cover_rect;
    bool cursor_on_cover;

    QRect artist_label_rect;
    bool cursor_on_artist_label;

    QRect title_label_rect;
    bool cursor_on_title_label;

    QRect year_label_rect;
    bool cursor_on_year_label;

    void setup_actions();

    bool enabled;

    QTemporaryDir tmp_dir;
};

#endif
