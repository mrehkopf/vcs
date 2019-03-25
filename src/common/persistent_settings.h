/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef INI_H
#define INI_H

#include <QVariant>

// Group names for the settings ini file.
#define INI_GROUP_FORCE_INPUT_BUTTONS   "INPUT_RES_BUTTONS"
#define INI_GROUP_OUTPUT_FILTERS        "OUTPUT_FILTERS"
#define INI_GROUP_CONTROL_PANEL         "CONTROL_PANEL"
#define INI_GROUP_ANTI_TEAR             "ANTI_TEAR"
#define INI_GROUP_RENDERER              "RENDERER"
#define INI_GROUP_RECORDING             "RECORDING"
#define INI_GROUP_GEOMETRY              "DIALOG_GEOMETRY"
#define INI_GROUP_OVERLAY               "OVERLAY"
#define INI_GROUP_LOG                   "LOG"

QVariant kpers_value_of(const QString &group, const QString &name, const QVariant &defaultValue = QVariant());

void kpers_set_value(const QString &group, const QString &name, const QVariant &value);

bool kpers_contains(const QString &group, const QString &name);

#endif
