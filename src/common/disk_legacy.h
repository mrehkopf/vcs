#ifndef DISK_LEGACY_H_
#define DISK_LEGACY_H_

#include <QString>
#include <string>

struct legacy14_filter_set_s;

bool kdisk_legacy14_save_filter_sets(const std::vector<legacy14_filter_set_s*>& filterSets, const QString &targetFilename);
std::vector<legacy14_filter_set_s*> kdisk_legacy14_load_filter_sets(const std::string &sourceFilename);

#endif
