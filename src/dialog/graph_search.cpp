#include "dialog/graph_search.h"

#include <memory>
#include <unordered_set>

#include "base/debug.h"
#include "engine/Tileset.h"

using babel::engine::GameState;
using babel::engine::Tile;
using std::unique_ptr;
using std::unordered_set;
using std::vector;

namespace babel {
namespace dialog {
namespace {

static const Point kKingMoves[] = {
    {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};

}  // namespace

vector<Point> GetBlockingSquares(
    const GameState& game_state, const vector<Point>& squares) {
  vector<Point> result;
  unordered_set<Point> visited;
  for (const Point& square : squares) {
    visited.insert(square);
  }
  unique_ptr<vector<Point>> queue(new vector<Point>(squares));
  while (queue->size() > 0) {
    unique_ptr<vector<Point>> next(new vector<Point>);
    for (const Point& square : *queue) {
      for (const Point& move : kKingMoves) {
        const Point neighbor = square + move;
        if (visited.find(neighbor) == visited.end()) {
          visited.insert(neighbor);
          const Tile tile = game_state.map->GetTile(neighbor);
          if (tile == Tile::FREE) {
            result.push_back(neighbor);
          } else if (!game_state.map->IsSquareBlocked(neighbor)) {
            next->push_back(neighbor);
          }
        }
      }
    }
    queue = std::move(next);
  }
  return result;
}

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
          if (!game_state.map->IsSquareBlocked(neighbor) &&
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
