#ifndef DISK_LEGACY_H_
#define DISK_LEGACY_H_

#include <QString>
#include <string>
#include <vector>

struct legacy14_filter_set_s;

std::vector<legacy14_filter_set_s*> kdisk_legacy14_load_filter_sets(const std::string &sourceFilename);

#endif
