/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef VCS_COMMON_DISK_FILE_READERS_FILE_READER_FILTER_GRAPH_H
#define VCS_COMMON_DISK_FILE_READERS_FILE_READER_FILTER_GRAPH_H

#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "display/display.h"

namespace file_reader
{
namespace filter_graph
{
    namespace version_a
    {
        bool read(const std::string &filename,
                  std::vector<abstract_filter_graph_node_s> * const graphNodes);
    }

    namespace version_b
    {
        bool read(const std::string &filename,
                  std::vector<abstract_filter_graph_node_s> *const graphNodes);
    }
}
}

#endif
