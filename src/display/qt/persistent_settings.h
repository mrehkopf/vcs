/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef VCS_DISPLAY_QT_PERSISTENT_SETTINGS_H
#define VCS_DISPLAY_QT_PERSISTENT_SETTINGS_H

#include <QVariant>

// Group names for the settings ini file.
#define INI_GROUP_INPUT                 "INPUT"
#define INI_GROUP_OUTPUT                "OUTPUT"
#define INI_GROUP_ANTI_TEAR             "ANTI_TEAR"
#define INI_GROUP_RECORDING             "RECORDING"
#define INI_GROUP_GEOMETRY              "DIALOG_GEOMETRY"
#define INI_GROUP_OVERLAY               "OVERLAY"
#define INI_GROUP_APP                   "APP"
#define INI_GROUP_VIDEO_PRESETS         "VIDEO_PRESETS"
#define INI_GROUP_FILTER_GRAPH          "FILTER_GRAPH"
#define INI_GROUP_ALIAS_RESOLUTIONS     "ALIAS_RESOLUTIONS"

QVariant kpers_value_of(const QString &group, const QString &name, const QVariant &defaultValue = QVariant());

void kpers_set_value(const QString &group, const QString &name, const QVariant &value);

bool kpers_contains(const QString &group, const QString &name);

#endif
