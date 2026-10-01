/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Test runner: executes every test registered with AUDEX_TEST. With command
// line arguments only the tests whose name contains one of them run, e.g.
//     audex_tests offset cache

#include "test_framework.h"

#include <QStringList>

int main(int argc, char **argv)
{
    using namespace Audex::Tests;

    QStringList filters;
    for (int i = 1; i < argc; ++i)
        filters.append(QString::fromLocal8Bit(argv[i]));

    Context context;
    int ran = 0;
    for (const TestCase &test : Context::registry()) {
        if (!filters.isEmpty()) {
            bool wanted = false;
            for (const QString &filter : filters)
                wanted = wanted || QString::fromLatin1(test.name).contains(filter, Qt::CaseInsensitive);
            if (!wanted)
                continue;
        }

        ++ran;
        context.currentTest = test.name;
        const int failuresBefore = context.failures;
        test.function(context);
        if (context.failures == failuresBefore)
            qInfo().noquote() << "PASS" << test.name;
    }

    qInfo().noquote() << QString("%1 test(s), %2 check(s), %3 failure(s)").arg(ran).arg(context.checks).arg(context.failures);
    return context.failures == 0 ? 0 : 1;
}
