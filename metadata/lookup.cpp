/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lookup.h"

#include <QPointer>
#include <QTimer>

using namespace Qt::StringLiterals;

namespace Audex
{

// ---- PrecomputedProvider ---------------------------------------------------------

PrecomputedProvider::PrecomputedProvider(const QString &id, const QString &displayName, QObject *parent)
    : MetadataProvider(parent)
    , m_id(id)
    , m_displayName(displayName)
{
}

void PrecomputedProvider::setCandidates(const Cdda::Toc &toc, const MetadataCandidates &candidates)
{
    m_toc = toc;
    m_candidates = candidates;
}

void PrecomputedProvider::clear()
{
    m_toc = Cdda::Toc();
    m_candidates.clear();
}

void PrecomputedProvider::lookup(int requestId, const CDInfo &disc)
{
    const MetadataCandidates result = (!m_toc.isEmpty() && disc.toc() == m_toc) ? m_candidates : MetadataCandidates();
    QTimer::singleShot(0, this, [this, requestId, result] {
        if (m_canceled.remove(requestId))
            return;
        Q_EMIT finished(requestId, result, QString());
    });
}

void PrecomputedProvider::cancel(int requestId)
{
    m_canceled.insert(requestId);
}

// ---- MetadataLookup ----------------------------------------------------------------

MetadataLookup::MetadataLookup(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Audex::MetadataCandidates>();
}

void MetadataLookup::addProvider(MetadataProvider *provider)
{
    Q_ASSERT(provider);
    provider->setParent(this);
    m_providers.append(provider);
    const QPointer<MetadataProvider> guard(provider);
    connect(provider, &MetadataProvider::finished, this, [this, guard](int requestId, const MetadataCandidates &candidates, const QString &error) {
        if (guard)
            onProviderFinished(guard, requestId, candidates, error);
    });
    // A provider that disappears must not leave lookups hanging
    connect(provider, &QObject::destroyed, this, [this, provider] {
        m_providers.removeAll(provider);
        const QList<int> requests = m_requestProvider.keys(provider);
        for (int requestId : requests) {
            m_requestProvider[requestId] = nullptr;
            onProviderFinished(nullptr, requestId, {}, u"Provider was deleted"_s);
        }
    });
}

QList<MetadataProvider *> MetadataLookup::providers() const
{
    return m_providers;
}

MetadataProvider *MetadataLookup::provider(const QString &id) const
{
    for (MetadataProvider *p : m_providers)
        if (p->id() == id)
            return p;
    return nullptr;
}

void MetadataLookup::setProviderEnabled(const QString &id, bool enabled)
{
    if (enabled)
        m_disabled.remove(id);
    else
        m_disabled.insert(id);
}

bool MetadataLookup::isProviderEnabled(const QString &id) const
{
    return !m_disabled.contains(id);
}

int MetadataLookup::start(const CDInfo &disc, const QStringList &providerIds)
{
    const int lookupId = m_nextLookupId++;
    Running &running = m_running[lookupId];

    for (MetadataProvider *p : std::as_const(m_providers)) {
        const bool selected = providerIds.isEmpty() ? (isProviderEnabled(p->id()) && p->isAvailable()) : providerIds.contains(p->id());
        if (!selected)
            continue;
        const int requestId = m_nextRequestId++;
        running.pendingRequests.insert(requestId);
        m_requestToLookup.insert(requestId, lookupId);
        m_requestProvider.insert(requestId, p);
        m_requestProviderId.insert(requestId, p->id());

        // start asynchronously, so the caller knows the id before any signal
        const QPointer<MetadataProvider> guard(p);
        QTimer::singleShot(0, this, [this, guard, requestId, disc] {
            if (!m_requestToLookup.contains(requestId))
                return; // canceled before it started
            if (!guard)
                return; // already failed through destroyed()
            guard->lookup(requestId, disc);
        });
    }

    if (running.pendingRequests.isEmpty()) {
        QTimer::singleShot(0, this, [this, lookupId] {
            finishIfDone(lookupId);
        });
    }
    return lookupId;
}

void MetadataLookup::cancel(int lookupId)
{
    const auto it = m_running.find(lookupId);
    if (it == m_running.end())
        return;
    for (int requestId : std::as_const(it->pendingRequests)) {
        m_requestToLookup.remove(requestId);
        m_requestProviderId.remove(requestId);
        if (MetadataProvider *p = m_requestProvider.take(requestId))
            p->cancel(requestId);
    }
    m_running.erase(it);
}

bool MetadataLookup::isRunning(int lookupId) const
{
    return m_running.contains(lookupId);
}

void MetadataLookup::onProviderFinished(MetadataProvider *provider, int requestId, const MetadataCandidates &candidates, const QString &error)
{
    const auto lookupIt = m_requestToLookup.constFind(requestId);
    if (lookupIt == m_requestToLookup.cend())
        return; // canceled or unknown
    const int lookupId = lookupIt.value();
    m_requestToLookup.remove(requestId);
    m_requestProvider.remove(requestId);
    const QString storedProviderId = m_requestProviderId.take(requestId);

    auto runningIt = m_running.find(lookupId);
    if (runningIt == m_running.end())
        return;
    runningIt->pendingRequests.remove(requestId);

    MetadataCandidates received = candidates;
    for (MetadataCandidate &c : received) {
        if (c.provider.isEmpty() && provider)
            c.provider = provider->id();
        if (c.providerName.isEmpty() && provider)
            c.providerName = provider->displayName();
    }
    runningIt->candidates += received;

    const QString providerId = provider ? provider->id() : storedProviderId;
    if (!received.isEmpty())
        Q_EMIT candidatesFound(lookupId, received);
    if (!error.isEmpty())
        Q_EMIT providerFailed(lookupId, providerId, error);

    finishIfDone(lookupId);
}

void MetadataLookup::finishIfDone(int lookupId)
{
    const auto it = m_running.find(lookupId);
    if (it == m_running.end() || !it->pendingRequests.isEmpty())
        return;
    MetadataCandidates all = it->candidates;
    m_running.erase(it);
    sortCandidates(all);
    Q_EMIT finished(lookupId, all);
}

}
