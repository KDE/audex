/* AUDEX CDDA EXTRACTOR
 * SPDX-FileCopyrightText: Copyright (C) 2007-2026 Marco Nelles
 * <https://userbase.kde.org/Audex>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "devicesettings.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include "preferences.h"

namespace
{

const char KEY_SECURE_MODE[] = "secureMode";
const char KEY_USE_C2[] = "useC2";
const char KEY_CACHE_DEFEAT[] = "cacheDefeat";
const char KEY_OVERREAD[] = "overread";
const char KEY_RETRIES[] = "retriesOnReadError";
const char KEY_SAMPLE_SHIFT[] = "sampleShift";
const char KEY_READ_SPEED[] = "readSpeed";
const char KEY_ERROR_READ_SPEED[] = "errorReadSpeed";
const char KEY_CACHE_DEFEAT_READS[] = "cacheDefeatReads";

// drive assistant
const char KEY_FEATURES_MEASURED[] = "featuresMeasured";
const char KEY_BURST_SECTORS[] = "burstSectors";
const char KEY_ACCURATE_STREAM[] = "accurateStream";
const char KEY_JITTER[] = "jitterSamples";
const char KEY_CACHING[] = "caching";
const char KEY_C2_SUPPORTED[] = "c2Supported";
const char KEY_C2_RELIABLE[] = "c2Reliable";
const char KEY_LEAD_IN[] = "leadInReadable";
const char KEY_LEAD_OUT[] = "leadOutReadable";
const char KEY_SUBCHANNEL_Q[] = "subchannelQ";
const char KEY_RW_SUBCHANNEL[] = "rwSubchannel";
const char KEY_RW_SHIFT[] = "rwShift";

QString groupName(const QString &udi)
{
    return QStringLiteral("Device ") + udi;
}

// a measured feature is stored as the number of its enumerator
Audex::Rip::Feature readFeature(const KConfigGroup &group, const char *key)
{
    const int value = group.readEntry(key, int(Audex::Rip::Feature::Unknown));
    switch (value) {
    case int(Audex::Rip::Feature::Yes):
        return Audex::Rip::Feature::Yes;
    case int(Audex::Rip::Feature::No):
        return Audex::Rip::Feature::No;
    default:
        return Audex::Rip::Feature::Unknown;
    }
}

}

DeviceSettings::Values DeviceSettings::defaults()
{
    return Values{.secureMode = Preferences::secureMode(),
                  .useC2 = Preferences::useC2(),
                  .cacheDefeat = Preferences::cacheDefeat(),
                  .overread = Preferences::overread(),
                  .retriesOnReadError = Preferences::retriesOnReadError(),
                  .sampleShift = Preferences::sampleShift(),
                  .readSpeed = Preferences::readSpeed(),
                  .errorReadSpeed = Preferences::errorReadSpeed()};
}

DeviceSettings::Values DeviceSettings::factoryDefaults()
{
    const auto value = [](const char *key) {
        return Preferences::self()->findItem(QString::fromLatin1(key))->getDefault();
    };
    return Values{.secureMode = value(KEY_SECURE_MODE).toBool(),
                  .useC2 = value(KEY_USE_C2).toBool(),
                  .cacheDefeat = value(KEY_CACHE_DEFEAT).toBool(),
                  .overread = value(KEY_OVERREAD).toBool(),
                  .retriesOnReadError = value(KEY_RETRIES).toInt(),
                  .sampleShift = value(KEY_SAMPLE_SHIFT).toInt(),
                  .readSpeed = value(KEY_READ_SPEED).toInt(),
                  .errorReadSpeed = value(KEY_ERROR_READ_SPEED).toInt()};
}

DeviceSettings::Values DeviceSettings::load(const QString &udi)
{
    const Values def = defaults();
    if (udi.isEmpty())
        return def;

    const KConfigGroup group(KSharedConfig::openConfig(), groupName(udi));
    Values values;
    values.secureMode = group.readEntry(KEY_SECURE_MODE, def.secureMode);
    values.useC2 = group.readEntry(KEY_USE_C2, def.useC2);
    values.cacheDefeat = group.readEntry(KEY_CACHE_DEFEAT, def.cacheDefeat);
    values.overread = group.readEntry(KEY_OVERREAD, def.overread);
    values.retriesOnReadError = group.readEntry(KEY_RETRIES, def.retriesOnReadError);
    values.sampleShift = group.readEntry(KEY_SAMPLE_SHIFT, def.sampleShift);
    values.readSpeed = group.readEntry(KEY_READ_SPEED, def.readSpeed);
    values.errorReadSpeed = group.readEntry(KEY_ERROR_READ_SPEED, def.errorReadSpeed);
    return values;
}

void DeviceSettings::save(const QString &udi, const Values &values)
{
    if (udi.isEmpty())
        return;

    KConfigGroup group(KSharedConfig::openConfig(), groupName(udi));
    group.writeEntry(KEY_SECURE_MODE, values.secureMode);
    group.writeEntry(KEY_USE_C2, values.useC2);
    group.writeEntry(KEY_CACHE_DEFEAT, values.cacheDefeat);
    group.writeEntry(KEY_OVERREAD, values.overread);
    group.writeEntry(KEY_RETRIES, values.retriesOnReadError);
    group.writeEntry(KEY_SAMPLE_SHIFT, values.sampleShift);
    group.writeEntry(KEY_READ_SPEED, values.readSpeed);
    group.writeEntry(KEY_ERROR_READ_SPEED, values.errorReadSpeed);
    group.sync();
}

int DeviceSettings::cacheDefeatReads(const QString &udi)
{
    if (udi.isEmpty())
        return 0;
    return KConfigGroup(KSharedConfig::openConfig(), groupName(udi)).readEntry(KEY_CACHE_DEFEAT_READS, 0);
}

void DeviceSettings::setCacheDefeatReads(const QString &udi, int reads)
{
    if (udi.isEmpty())
        return;
    KConfigGroup group(KSharedConfig::openConfig(), groupName(udi));
    group.writeEntry(KEY_CACHE_DEFEAT_READS, reads);
    group.sync();
}

Audex::Rip::DriveFeatures DeviceSettings::features(const QString &udi)
{
    Audex::Rip::DriveFeatures f;
    if (udi.isEmpty())
        return f;

    const KConfigGroup group(KSharedConfig::openConfig(), groupName(udi));
    f.measured = group.readEntry(KEY_FEATURES_MEASURED, QDateTime());
    f.burstSectors = group.readEntry(KEY_BURST_SECTORS, 0);
    f.accurateStream = readFeature(group, KEY_ACCURATE_STREAM);
    f.jitterSamples = group.readEntry(KEY_JITTER, 0);
    f.caching = readFeature(group, KEY_CACHING);
    f.cacheDefeatReads = group.readEntry(KEY_CACHE_DEFEAT_READS, 0);
    f.c2Supported = group.readEntry(KEY_C2_SUPPORTED, false);
    f.c2Reliable = readFeature(group, KEY_C2_RELIABLE);
    f.leadIn = readFeature(group, KEY_LEAD_IN);
    f.leadOut = readFeature(group, KEY_LEAD_OUT);
    f.subchannelQ = readFeature(group, KEY_SUBCHANNEL_Q);
    f.rwSubchannel = readFeature(group, KEY_RW_SUBCHANNEL);
    f.rwShift = group.readEntry(KEY_RW_SHIFT, 0);
    return f;
}

void DeviceSettings::setFeatures(const QString &udi, const Audex::Rip::DriveFeatures &features)
{
    if (udi.isEmpty() || !features.measured.isValid())
        return;

    KConfigGroup group(KSharedConfig::openConfig(), groupName(udi));
    group.writeEntry(KEY_FEATURES_MEASURED, features.measured);
    group.writeEntry(KEY_BURST_SECTORS, features.burstSectors);
    group.writeEntry(KEY_ACCURATE_STREAM, int(features.accurateStream));
    group.writeEntry(KEY_JITTER, features.jitterSamples);
    group.writeEntry(KEY_CACHING, int(features.caching));
    group.writeEntry(KEY_C2_SUPPORTED, features.c2Supported);
    group.writeEntry(KEY_C2_RELIABLE, int(features.c2Reliable));
    group.writeEntry(KEY_LEAD_IN, int(features.leadIn));
    group.writeEntry(KEY_LEAD_OUT, int(features.leadOut));
    group.writeEntry(KEY_SUBCHANNEL_Q, int(features.subchannelQ));
    group.writeEntry(KEY_RW_SUBCHANNEL, int(features.rwSubchannel));
    group.writeEntry(KEY_RW_SHIFT, features.rwShift);
    if (features.caching == Audex::Rip::Feature::Yes)
        group.writeEntry(KEY_CACHE_DEFEAT_READS, features.cacheDefeatReads);
    group.sync();
}
