/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

#include "candidate.h"
#include "cdinfo.h"

namespace Audex
{

// A source of metadata (MusicBrainz, CDDB via libkcddb, CD-Text, a local
// database, ...). Providers are asynchronous. Contract:
//  - lookup() starts a request and returns immediately
//  - finished() is emitted exactly once per request, never from within lookup()
//  - after cancel() nothing is emitted for that request
//  - "nothing found" is an empty candidate list, not an error
class MetadataProvider : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool isAvailable() const
    {
        return true;
    }

    virtual void lookup(int requestId, const CDInfo &disc) = 0;
    virtual void cancel(int requestId)
    {
        Q_UNUSED(requestId)
    }

Q_SIGNALS:
    void finished(int requestId, const Audex::MetadataCandidates &candidates, const QString &error);
};

// Delivers candidates that are already known, e.g. CD-Text read together with
// the TOC. The candidates are only returned for the disc they belong to.
class PrecomputedProvider : public MetadataProvider
{
    Q_OBJECT

public:
    PrecomputedProvider(const QString &id, const QString &displayName, QObject *parent = nullptr);

    QString id() const override
    {
        return m_id;
    }
    QString displayName() const override
    {
        return m_displayName;
    }

    void setCandidates(const Cdda::Toc &toc, const MetadataCandidates &candidates);
    void clear();

    void lookup(int requestId, const CDInfo &disc) override;
    void cancel(int requestId) override;

private:
    QString m_id;
    QString m_displayName;
    Cdda::Toc m_toc;
    MetadataCandidates m_candidates;
    QSet<int> m_canceled;
};

// Runs all (or selected) providers in parallel and collects the candidates.
// Each lookup has its own id, so several drives can be served at once.
class MetadataLookup : public QObject
{
    Q_OBJECT

public:
    explicit MetadataLookup(QObject *parent = nullptr);

    void addProvider(MetadataProvider *provider); // takes ownership
    QList<MetadataProvider *> providers() const;
    MetadataProvider *provider(const QString &id) const;

    void setProviderEnabled(const QString &id, bool enabled);
    bool isProviderEnabled(const QString &id) const;

    // Returns the lookup id; all signals are delivered asynchronously.
    // providerIds empty = all enabled and available providers.
    int start(const CDInfo &disc, const QStringList &providerIds = QStringList());
    void cancel(int lookupId);
    bool isRunning(int lookupId) const;

Q_SIGNALS:
    // Candidates of one provider, as soon as they arrive
    void candidatesFound(int lookupId, const Audex::MetadataCandidates &candidates);
    void providerFailed(int lookupId, const QString &providerId, const QString &error);
    // All providers are done; candidates sorted by score
    void finished(int lookupId, const Audex::MetadataCandidates &candidates);

private:
    void onProviderFinished(MetadataProvider *provider, int requestId, const MetadataCandidates &candidates, const QString &error);
    void finishIfDone(int lookupId);

    struct Running {
        QSet<int> pendingRequests;
        MetadataCandidates candidates;
    };

    QList<MetadataProvider *> m_providers;
    QSet<QString> m_disabled;
    QHash<int, Running> m_running;
    QHash<int, int> m_requestToLookup;
    QHash<int, MetadataProvider *> m_requestProvider;
    QHash<int, QString> m_requestProviderId;
    int m_nextLookupId = 1;
    int m_nextRequestId = 1;
};

}
