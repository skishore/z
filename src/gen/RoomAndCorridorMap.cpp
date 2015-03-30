#include "gen/RoomAndCorridorMap.h"

#include <algorithm>

#include "base/debug.h"
#include "base/util.h"
#include "engine/Tileset.h"
#include "engine/TileMap.h"
#include "gen/graph.h"
#include "gen/util.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Tileset;
using std::min;
using std::vector;

namespace babel {
namespace gen {
namespace {

#define MAYBE_DEBUG(...) if (verbose) { DEBUG(__VA_ARGS__); }

// The caller takes ownership of the new tileset.
Tileset* DefaultTileset() {
  return new Tileset{4 /* blocked_tile */, 5 /* default_tile */};
}

}  // namespace

RoomAndCorridorMap::RoomAndCorridorMap(const Point& size, bool verbose) {
  size_ = size;
  tileset_.reset(DefaultTileset());

  CellArray cells = ConstructArray2d<Cell>(size_, Cell::DEFAULT);
  Array2d<bool> diggable = ConstructArray2d<bool>(size_, true);

  const int min_size = 6;
  const int max_size = 8;
  const int separation = 3;
  const int tries = size_.x*size_.y/(min_size*min_size);
  int tries_left = tries;

  while (tries_left > 0) {
    const Point size{RandInt(min_size, max_size), RandInt(min_size, max_size)};
    const Room room{{RandInt(1, size_.x - size.x - 1),
                     RandInt(1, size_.y - size.y - 1)}, size};
    if (!PlaceRoom(room, separation, &cells, &diggable, &rooms_)) {
      tries_left -= 1;
    }
  }
  const int n = rooms_.size();
  MAYBE_DEBUG("Placed " << IntToString(n) << " rooms after "
              << IntToString(tries) << " attempts.");

  Array2d<double> graph = ConstructArray2d<double>(Point(n, n), 0);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      graph[i][j] = RoomToRoomDistance(rooms_[i], rooms_[j]);
      ASSERT(i == j || graph[i][j] > 0, "Rooms are touching!");
    }
  }
  vector<Point> edges = MinimumSpanningTree(graph);
  MAYBE_DEBUG("Computed a minimal spanning tree with "
              << IntToString(edges.size()) << " edges.");

  int loop_edges = 0;
  Array2d<double> distances = ComputeTreeDistances(graph, edges);
  while (true) {
    Point best_edge;
    double best_ratio = 0;
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        double ratio = distances[i][j]/graph[i][j];
        if (ratio > best_ratio) {
          best_edge = Point(i, j);
          best_ratio = ratio;
        }
      }
    }
    if (best_ratio < 3.0) {
      break;
    }
    edges.push_back(best_edge);
    double distance = graph[best_edge.x][best_edge.y];
    for (int i = 0; i < n; i++) {
      for (int j = 0; j < n; j++) {
        distances[i][j] = min(distances[i][j], min(
            distances[i][best_edge.x] + distance + distances[best_edge.y][j],
            distances[i][best_edge.y] + distance + distances[best_edge.x][j]));
      }
    }
    loop_edges += 1;
  }
  MAYBE_DEBUG("Added " << IntToString(loop_edges) << " high-ratio loop edges.");

  for (const Point& edge : edges) {
    ASSERT(edge.x != edge.y, "Graph includes reflexive edge!");
    DigCorridor(rooms_[edge.x], rooms_[edge.y], size_, &cells, &diggable);
  }
  MAYBE_DEBUG("Dug " << IntToString(edges.size()) << " corridors.");

  MAYBE_DEBUG("Final map:" << ComputeDebugString(cells));
  tiles_.reset(ComputeTiles(*tileset_, cells));
}

}  // namespace gen
}  // namespace babel
