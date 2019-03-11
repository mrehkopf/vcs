/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 * A basic convenience wrapper for QSettings.
 *
 */

#include <QSettings>
#include "persistent_settings.h"

static const char SETTINGS_FILENAME[] = "vcs.ini";

static QSettings SETTINGS_FILE(SETTINGS_FILENAME, QSettings::IniFormat);

void kpers_initialize(void)
{
    /// Nothing at the moment.

    return;
}

// Concatenates the given name and group into a key usable with QSettings's value()
// and setValue().
//
static QString as_key(const QString &name, const QString &group)
{
    return QString("%1/%2").arg(group.toUpper()).arg(name.toUpper());
}

QVariant kpers_value_of(const QString &name, const QString &group, const QVariant &defaultValue)
{
    const QVariant v = SETTINGS_FILE.value(as_key(name, group), defaultValue);

    return v;
}

bool kpers_contains(const QString &name, const QString &group)
{
    return SETTINGS_FILE.contains(as_key(name, group));
}

void kpers_set_value(const QString &name, const QString &group, const QVariant &value)
{
    SETTINGS_FILE.setValue(as_key(name, group), value);

    return;
}
