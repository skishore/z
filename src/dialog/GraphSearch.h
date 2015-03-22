#ifndef __BABEL_DIALOG_GRAPH_SEARCH_H__
#define __BABEL_DIALOG_GRAPH_SEARCH_H__

#include <vector>

#include "base/point.h"
#include "engine/GameState.h"

namespace babel {
namespace dialog {

// Searches for n unblocked, unoccupied squares reachable from start.
// The search terminates at a distance of at most k.
std::vector<Point> GetReachableSquares(
    const engine::GameState& game_state, const Point& start, int n, int k);

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_GRAPH_SEARCH_H__
