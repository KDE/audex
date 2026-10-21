/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "enginetranslator.h"

#include <KLocalizedString>

#include <cstring>

bool EngineTranslator::isEmpty() const
{
    return false;
}

bool EngineTranslator::isEngineContext(const char *context)
{
    if (!context)
        return false;
    if (std::strncmp(context, "Audex::", 7) == 0)
        return true;
    // the encoder plugins
    for (const char *plugin : {"AiffEncoder", "FlacEncoder", "LameEncoder", "OpusEncoder"})
        if (std::strcmp(context, plugin) == 0)
            return true;
    return false;
}

QString EngineTranslator::translate(const char *context, const char *sourceText, const char *disambiguation, int n) const
{
    Q_UNUSED(n)
    if (!sourceText || !isEngineContext(context))
        return {}; // Qt's own texts and anything else: the other translators
    KLocalizedString text = disambiguation && *disambiguation ? ki18ndc("audex", disambiguation, sourceText) : ki18nd("audex", sourceText);
    // "%1" for %1 and so on: the placeholders stay for arg(), without a
    // complaint about missing arguments
    for (int i = 1; i <= 9; ++i)
        text = text.subs(QLatin1Char('%') + QString::number(i));
    return text.relaxSubs().toString();
}
