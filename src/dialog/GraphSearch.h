#ifndef __BABEL_DIALOG_GRAPH_SEARCH_H__
#define __BABEL_DIALOG_GRAPH_SEARCH_H__

#include <vector>

#include "base/point.h"
#include "engine/GameState.h"

namespace babel {
namespace dialog {

// Returns the list of unblocked and unoccupied squares reachable in up to k
// steps from the starting square.
std::vector<Point> GetReachableSquares(
    const engine::GameState& game_state, const Point& start, int k);

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_GRAPH_SEARCH_H__
