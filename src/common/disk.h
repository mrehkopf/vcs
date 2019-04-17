#ifndef DISK_H_
#define DISK_H_

#include <vector>

class QString;
struct mode_params_s;
struct mode_alias_s;
struct filter_set_s;

bool kdisk_save_mode_params(const std::vector<mode_params_s> &modeParams, const QString &targetFilename);
bool kdisk_save_filter_sets(const std::vector<filter_set_s*>& filterSets, const QString &targetFilename);
bool kdisk_save_aliases(const std::vector<mode_alias_s> &aliases, const QString &targetFilename);

bool kdisk_load_filter_sets(const std::string &sourceFilename);
bool kdisk_load_mode_params(const std::string &sourceFilename);
bool kdisk_load_aliases(const std::string &sourceFilename);

#endif
