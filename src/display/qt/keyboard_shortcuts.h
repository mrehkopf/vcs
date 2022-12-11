/*
 * 2022 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_DISPLAY_QT_KEYBOARD_SHORTCUTS_H
#define VCS_DISPLAY_QT_KEYBOARD_SHORTCUTS_H

class QKeySequence;
class QShortcut;

const QKeySequence& kd_get_key_sequence(const std::string &label);

QShortcut* kd_make_key_shortcut(const std::string &label, QWidget *const parent);

#endif
