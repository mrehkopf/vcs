/*
 * 2018 Tarpeeksi Hyvae Soft /
 * VCS
 *
 */

#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

class QString;

bool kcom_parse_command_line(const int argc, char *const argv[]);

const QString& kcom_alias_file_name(void);

const QString& kcom_filters_file_name(void);

const QString& kcom_params_file_name(void);

#endif
