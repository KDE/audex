/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QTranslator>

// The engine and the encoder plugins depend on Qt only and mark their texts
// with QCoreApplication::translate() (contexts "Audex::..." and
// "<Format>Encoder"). This translator looks them up in the audex gettext
// catalog, like i18n() does for the rest of the program. The %1 placeholders
// stay in the result: the engine fills them in itself with arg().
//
// Messages.sh extracts these texts with the keywords tr and translate:2.
class EngineTranslator : public QTranslator
{
    Q_OBJECT

public:
    using QTranslator::QTranslator;

    bool isEmpty() const override;
    QString translate(const char *context, const char *sourceText, const char *disambiguation = nullptr, int n = -1) const override;

    static bool isEngineContext(const char *context);
};
