/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2007-2025 Marco Nelles <marco.nelles@kdemail.net>
 * <https://apps.kde.org/audex/>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>

#include <KActionCollection>
#include <KColorScheme>
#include <KLocalizedString>

#include "datatypes/metadata.h"
#include "datatypes/toc.h"
#include "dialogs/errordialog.h"

// fixed point defines
#define FP_BITS 10
#define FP_FACTOR (1 << FP_BITS)

namespace Audex
{

enum FadeStyle {
    NoFade,
    FadeDown,
    FadeRight,
    FadeUp,
    FadeLeft
};

enum MirrorStyle {
    NoMirror,
    MirrorOverX,
    MirrorOverY
};

class CDDAHeaderWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CDDAHeaderWidget(QWidget *parent = nullptr, const int cover_size_min = 200, const int cover_size_max = 400, const int padding = 20);
    ~CDDAHeaderWidget() override;
    QSize sizeHint() const override;

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enabled);
    void editData();
    void updateMetadata(const Audex::Metadata::Metadata &metadata);
    void update(const Audex::Metadata::Metadata &metadata, const Audex::Toc::Toc &toc);

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
    Audex::Metadata::Metadata metadata;
    Audex::Toc::Toc toc;

    QPointer<KActionCollection> action_collection;
    int cover_size_min;
    int cover_size_max;
    int padding;

    QImage cd_case;
    void construct_cd_case();

    QRect cover_rect;
    bool cursor_on_cover;

    QRect label_rect;
    bool cursor_on_label;

    void setup_actions();

    bool enabled;

    QTemporaryDir tmp_dir;
};

}
