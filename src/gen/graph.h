#ifndef __BABEL_GEN_GRAPH_H__
#define __BABEL_GEN_GRAPH_H__

#include <vector>

#include "base/point.h"

namespace babel {
namespace gen {

typedef std::vector<std::vector<double>> Graph;

std::vector<Point> MinimumSpanningTree(const Graph& graph);

Graph ComputeTreeDistances(
    const Graph& graph, const std::vector<Point>& tree);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_GRAPH_H__
