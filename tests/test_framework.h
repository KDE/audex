/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: 2026 Marco Nelles
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

// Minimal test framework for the engine tests, independent of Qt Test:
//  - AUDEX_TEST("name") registers a test case (no list to maintain)
//  - AUDEX_CHECK / AUDEX_CHECK_MSG assert a condition
//  - AUDEX_EQUAL asserts equality of two values and prints both on failure
//  - AUDEX_EQUAL_DATA compares two QByteArrays and reports sizes and the
//    first differing byte (with the sector it belongs to)
// The runner (test_main.cpp) executes all registered tests, or only those
// whose name contains one of the command line arguments.

#pragma once

#include <QByteArray>
#include <QDebug>
#include <QList>
#include <QString>
#include <QStringList>

#include <algorithm>

namespace Audex::Tests
{

class Context;

struct TestCase {
    const char *name;
    void (*function)(Context &);
};

// Values are printed with these helpers on failure; add overloads as needed.
inline QString describe(qint64 value)
{
    return QString::number(value);
}

inline QString describe(const QByteArray &data)
{
    return QString::number(data.size()) + QString(" bytes");
}

inline QString describe(const QString &text)
{
    return QLatin1Char('"') + text + QLatin1Char('"');
}

inline QString describe(const QStringList &list)
{
    return QLatin1Char('[') + list.join(QLatin1Char('|')) + QLatin1Char(']');
}

class Context
{
public:
    static QList<TestCase> &registry()
    {
        static QList<TestCase> tests;
        return tests;
    }

    void check(bool condition, const char *expression, const char *file, int line, const QString &message = {})
    {
        ++checks;
        if (condition)
            return;

        ++failures;
        qCritical().noquote() << "FAIL" << currentTest << "—" << file << ':' << line << ": CHECK(" << expression << ')';
        if (!message.isEmpty())
            qCritical().noquote() << "    " << message;
    }

    template<typename A, typename B>
    void equal(const A &actual, const B &expected, const char *actualExpression, const char *expectedExpression, const char *file, int line)
    {
        ++checks;
        if (actual == expected)
            return;

        ++failures;
        qCritical().noquote() << "FAIL" << currentTest << "—" << file << ':' << line << ": EQUAL(" << actualExpression << ','
                              << expectedExpression << ')';
        qCritical().noquote() << "     actual:" << describe(actual) << " expected:" << describe(expected);
    }

    void equalData(const QByteArray &actual, const QByteArray &expected, const char *actualExpression, const char *expectedExpression,
                   const char *file, int line)
    {
        ++checks;
        if (actual == expected)
            return;

        ++failures;
        qCritical().noquote() << "FAIL" << currentTest << "—" << file << ':' << line << ": EQUAL_DATA(" << actualExpression << ','
                              << expectedExpression << ')';
        if (actual.size() != expected.size())
            qCritical().noquote() << "     size:" << actual.size() << " expected:" << expected.size();
        const qsizetype n = std::min(actual.size(), expected.size());
        for (qsizetype i = 0; i < n; ++i) {
            if (actual.at(i) != expected.at(i)) {
                qCritical().noquote() << "     first difference at byte" << i << "(sector" << (i / 2352) << ')';
                break;
            }
        }
    }

    int checks = 0;
    int failures = 0;
    const char *currentTest = "";
};

} // namespace Audex::Tests

#define AUDEX_CHECK(ctx, condition) \
    (ctx).check((condition), #condition, __FILE__, __LINE__)

#define AUDEX_CHECK_MSG(ctx, condition, message) \
    (ctx).check((condition), #condition, __FILE__, __LINE__, (message))

#define AUDEX_EQUAL(ctx, actual, expected) \
    (ctx).equal((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define AUDEX_EQUAL_DATA(ctx, actual, expected) \
    (ctx).equalData((actual), (expected), #actual, #expected, __FILE__, __LINE__)

#define AUDEX_TEST_CONCAT_INNER(a, b) a##b
#define AUDEX_TEST_CONCAT(a, b) AUDEX_TEST_CONCAT_INNER(a, b)

#define AUDEX_TEST(name) \
    static void AUDEX_TEST_CONCAT(audexTest, __LINE__)(Audex::Tests::Context &t); \
    static const bool AUDEX_TEST_CONCAT(audexTestRegistered, __LINE__) = [] { \
        Audex::Tests::Context::registry().append({name, AUDEX_TEST_CONCAT(audexTest, __LINE__)}); \
        return true; \
    }(); \
    static void AUDEX_TEST_CONCAT(audexTest, __LINE__)(Audex::Tests::Context &t)
