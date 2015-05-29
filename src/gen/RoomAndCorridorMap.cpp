#include "gen/RoomAndCorridorMap.h"

#include <algorithm>

#include "base/debug.h"
#include "base/util.h"
#include "engine/Tileset.h"
#include "engine/TileMap.h"
#include "gen/graph.h"
#include "gen/util.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Graphic;
using babel::engine::Tile;
using babel::engine::Tileset;
using std::min;
using std::vector;

namespace babel {
namespace gen {
namespace {

#define MAYBE_DEBUG(...) if (verbose) { DEBUG(__VA_ARGS__); }

class DefaultTileset : public Tileset {
 public:
  Graphic GetGraphicForTile(Tile tile) const override {
    if (tile == Tile::FREE) {
      return rand() % 4;
    } else if (tile == Tile::WALL) {
      return 4;
    } else if (tile == Tile::DOOR) {
      return 7;
    } else if (tile == Tile::FENCE) {
      return 6;
    }
    return 5;
  }
};

}  // namespace

RoomAndCorridorMap::RoomAndCorridorMap(const Point& size, bool verbose) {
  while (!TryBuildMap(size, verbose)) {}
}

bool RoomAndCorridorMap::TryBuildMap(const Point& size, bool verbose) {
  size_ = size;
  tileset_.reset(new DefaultTileset());

  Level level(size_);
  vector<Rect> rects;

  const int min_size = 6;
  const int max_size = 8;
  const int separation = 3;
  const int tries = size_.x*size_.y/(min_size*min_size);
  int tries_left = tries;

  while (tries_left > 0) {
    const Point size{RandInt(min_size, max_size),
                     RandInt(min_size/2, max_size/2)};
    const Rect rect{size, {RandInt(1, size_.x - size.x - 1),
                           RandInt(1, size_.y - size.y - 1)}};
    if (!level.PlaceRectangularRoom(rect, separation, &rects)) {
      tries_left -= 1;
    }
  }
  const int n = rects.size();
  ASSERT(n > 0);
  MAYBE_DEBUG("Placed " << IntToString(n) << " rectangular rooms after "
              << IntToString(tries) << " attempts.");

  Array2d<double> graph = ConstructArray2d<double>(Point(n, n), 0);
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < n; j++) {
      graph[i][j] = RectToRectDistance(rects[i], rects[j]);
      ASSERT((i == j) == (graph[i][j] == 0));
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
    if (best_ratio < 2.0) {
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

  const double islandness = rand() % 3;
  for (int i = 0; i < 3; i++) {
    level.Erode(islandness);
  }
  level.ExtractFinalRooms(n, &rooms_);

  const double windiness = 1.0;
  for (const Point& edge : edges) {
    ASSERT(edge.x != edge.y);
    if (!level.DigCorridor(rooms_, edge.x, edge.y, windiness)) {
      MAYBE_DEBUG("Failed to dig corridor. Retrying...");
      return false;
    }
  }
  MAYBE_DEBUG("Dug " << IntToString(edges.size()) << " corridors.");

  level.AddWalls();
  starting_square_ = rooms_[0].GetRandomSquare();
  MAYBE_DEBUG("Final map:" << level.ToDebugString());
  PackTiles(level.tiles);
  return true;
}

}  // namespace gen
}  // namespace babel
