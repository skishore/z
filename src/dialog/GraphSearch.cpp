#include "dialog/GraphSearch.h"

#include <memory>
#include <unordered_set>

#include "base/debug.h"

using babel::engine::GameState;
using std::unique_ptr;
using std::unordered_set;
using std::vector;

namespace babel {
namespace dialog {
namespace {

static const Point kKingMoves[] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

}  // namespace

vector<Point> GetReachableSquares(
    const GameState& game_state, const Point& start, int n, int k) {
  vector<Point> result;
  unordered_set<Point> visited{start};
  unique_ptr<vector<Point>> queue(new vector<Point>{start});
  for (int i = 0; i < k; i++) {
    unique_ptr<vector<Point>> next(new vector<Point>);
    for (const Point& square : *queue) {
      for (const Point& move : kKingMoves) {
        const Point neighbor = square + move;
        if (visited.find(neighbor) == visited.end()) {
          visited.insert(neighbor);
          if (!game_state.map.IsSquareBlocked(neighbor) &&
              !game_state.IsSquareOccupied(neighbor)) {
            result.push_back(neighbor);
            next->push_back(neighbor);
          }
        }
      }
    }
    queue = std::move(next);
    if (result.size() > n) {
      break;
    }
  }
  return result;
}

}  // namespace dialog
}  // namespace babel
