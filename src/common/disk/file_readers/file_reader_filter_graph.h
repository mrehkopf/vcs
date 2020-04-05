/*
 * 2020 Tarpeeksi Hyvae Soft
 *
 * Software: VCS
 *
 */

#ifndef FILE_READER_FILTER_GRAPH_H
#define FILE_READER_FILTER_GRAPH_H

#include "display/qt/dialogs/filter_graph/filter_graph_node.h"
#include "display/display.h"

namespace file_reader
{
namespace filter_graph
{
    namespace version_a
    {
        bool read(const std::string &filename,
                  std::vector<FilterGraphNode*> *const graphNodes,
                  std::vector<filter_graph_option_s> *const graphOptions);
    }

    namespace version_b
    {
        bool read(const std::string &filename,
                  std::vector<FilterGraphNode*> *const graphNodes,
                  std::vector<filter_graph_option_s> *const graphOptions);
    }
}
}

#endif
