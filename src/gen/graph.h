#ifndef __BABEL_GEN_GRAPH_H__
#define __BABEL_GEN_GRAPH_H__

#include <vector>

#include "base/point.h"

namespace babel {
namespace gen {

std::vector<Point> MinimumSpanningTree(
    const std::vector<std::vector<double>>& graph);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_GRAPH_H__
