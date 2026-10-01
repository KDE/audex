/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "dialogs/errordialog.h"
#include "models/cdinfomodel.h"

#include <QImage>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QTemporaryDir>
#include <QWidget>

#include <optional>

class KActionCollection;

// fixed point defines
#define FP_BITS 10
#define FP_FACTOR (1 << FP_BITS)

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
    explicit CDDAHeaderWidget(Audex::CDInfoModel *cddaModel,
                              QWidget *parent = nullptr,
                              const int cover_size_min = 200,
                              const int cover_size_max = 400,
                              const int padding = 20);
    ~CDDAHeaderWidget() override;
    QSize sizeHint() const override;

    // HDCD detection result: empty = not checked (turned off, running, failed)
    void setHdcd(std::optional<bool> detected);
    std::optional<bool> hdcd() const
    {
        return m_hdcd;
    }

    // CD+G detection result, the same way
    void setCdg(std::optional<bool> detected);

    // Which tracks carry pre-emphasis and what that means (empty if none)
    static QString preEmphasisText(const Audex::CDInfo &info);

public Q_SLOTS:
    void setEnabled(bool enabled);
    void edit_data();

Q_SIGNALS:
    void headerDataChanged();

protected:
    bool event(QEvent *event) override; // tool tip of the cover
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private Q_SLOTS:

    void update();

    void choose_cover();
    void load();
    void save();
    void view_cover();
    void show_cover_source();
    void remove();

    void context_menu(const QPoint &point);

private:
    QPointer<Audex::CDInfoModel> cdda_model;
    KActionCollection *action_collection = nullptr;
    int cover_size_min;
    int cover_size_max;
    int padding;

    QImage cd_case;
    void construct_cd_case();
    QString coverToolTip() const; // where the cover comes from, its size

    QRect cover_rect;
    bool cursor_on_cover;

    QRect label_rect;
    bool cursor_on_label;

    void setup_actions();

    bool enabled;

    std::optional<bool> m_hdcd;
    std::optional<bool> m_cdg;

    void updateToolTip();

    QTemporaryDir tmp_dir;
};
