#include "gen/util.h"

#include <algorithm>
#include <cfloat>
#include <unordered_map>
#include <unordered_set>

#include "base/debug.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Graphic;
using babel::engine::Tile;
using std::max;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace babel {
namespace gen {
namespace {

const Point kRookMoves[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

// IMPORTANT: The king moves are arranged in increasing order of angle.
const Point kKingMoves[] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
                            {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

inline string GetDebugCharForTile(Tile tile) {
  if (tile == Tile::DEFAULT) {
    return " ";
  } else if (tile == Tile::FREE) {
    return ".";
  } else if (tile == Tile::WALL) {
    return "#";
  } else if (tile == Tile::DOOR) {
    return "+";
  }
  return "?";
}

inline bool InBounds(const Point& square, const Point& size) {
  return (0 < square.x && square.x < size.x - 1 &&
          0 < square.y && square.y < size.y - 1);
}

bool AtEdgeOfRoom(const Point& square, const Room& room) {
  return (max(max(square.x - room.position.x - room.size.x + 1,
                  room.position.x - square.x), 0) +
          max(max(square.y - room.position.y - room.size.y + 1,
                  room.position.y - square.y), 0)) == 1;
}

void AddDoor(const Point& square, const Room& room,
             TileArray* tiles, Array2d<bool>* diggable) {
  ASSERT(AtEdgeOfRoom(square, room));
  if (square.x == room.position.x - 1 ||
      square.x == room.position.x + room.size.x) {
    (*diggable)[square.x][square.y + 1] = false;
    (*diggable)[square.x][square.y - 1] = false;
  } else {
    (*diggable)[square.x + 1][square.y] = false;
    (*diggable)[square.x - 1][square.y] = false;
  }
  if (rand() % 2 == 0) {
    (*tiles)[square.x][square.y] = Tile::DOOR;
  }
}

bool IsTileBlocked(Tile tile) {
  return !(tile == Tile::FREE || tile == Tile::DOOR);
}

// Returns true if, during erosion, this square cannot be modified because doing
// so would change the orthogonal connectivity of the free squares around it.
bool SquareFixedByConnectivityConstraint(
    const TileArray& tiles, const Point& square) {
  int neighbors_blocked = 0;
  int min_unblocked_index = -1;
  int max_unblocked_index = -1;
  int gaps = 0;
  bool has_free_orthogonal_neighbor = false;
  for (int i = 0; i < 8; i++) {
    const Point& step = kKingMoves[i];
    if (IsTileBlocked(tiles[square.x + step.x][square.y + step.y])) {
      neighbors_blocked += 1;
      continue;
    }
    if (i % 2 == 0) {
      has_free_orthogonal_neighbor = true;
    }
    if (min_unblocked_index < 0) {
      min_unblocked_index = i;
      max_unblocked_index = i;
      continue;
    }
    if (i > max_unblocked_index + 1) {
      gaps += 1;
    }
    max_unblocked_index = i;
  }
  if (min_unblocked_index >= 0 &&
      !(min_unblocked_index == 0 && max_unblocked_index == 7)) {
    gaps += 1;
  }
  return neighbors_blocked == 8 || gaps > 1 || !has_free_orthogonal_neighbor;
}

}  // namespace

Level::Level(const Point& s)
    : size(s), tiles(ConstructArray2d<Tile>(s, Tile::DEFAULT)),
      rooms(ConstructArray2d<unsigned char>(s, 0)),
      diggable(ConstructArray2d<bool>(s, true)) {}

void Level::AddWalls() {
  for (int x = 0; x < size.x; x++) {
    for (int y = 0; y < size.y; y++) {
      if (tiles[x][y] != Tile::FREE) {
        continue;
      }
      for (const Point& step : kKingMoves) {
        const Point square(x + step.x, y + step.y);
        if (0 <= square.x && square.x < size.x &&
            0 <= square.y && square.y < size.y &&
            tiles[square.x][square.y] == Tile::DEFAULT) {
          tiles[square.x][square.y] = Tile::WALL;
        }
      }
    }
  }
}

void Level::DigCorridor(const Room& r1, const Room& r2, double windiness) {
  const Point source = GetRandomSquareInRoom(r1);
  const Point target = GetRandomSquareInRoom(r2);
  ASSERT(InBounds(source, size) && diggable[source.x][source.y]);
  ASSERT(InBounds(target, size) && diggable[target.x][target.y]);

  unordered_map<Point, double> distances{{source, 0}};
  unordered_map<Point, Point> parents;
  unordered_set<Point> visited;

  // Run Djikstra's algorithm between the source and target.
  while (visited.find(target) == visited.end()) {
    Point best_node;
    double best_distance = DBL_MAX;
    ASSERT(distances.size() > 0);
    for (const auto& pair : distances) {
      if (pair.second <= best_distance) {
        best_node = pair.first;
        best_distance = pair.second;
      }
    }
    distances.erase(best_node);
    visited.insert(best_node);
    for (const Point& step : kRookMoves) {
      const Point child = best_node + step;
      if (!(InBounds(child, size) && diggable[child.x][child.y] &&
            visited.find(child) == visited.end())) {
        continue;
      }
      const bool blocked = IsTileBlocked(tiles[child.x][child.y]);
      const double distance = best_distance + (blocked ? 2.0 : windiness);
      if (distances.find(child) == distances.end() ||
          distance < distances.at(child)) {
        distances[child] = distance;
        parents[child] = best_node;
      }
    }
  }

  // Construct the actual path from source to target.
  vector<Point> path;
  Point node = target;
  while (node != source) {
    path.push_back(node);
    node = parents.at(node);
  }

  // Truncate the path to only include sections outside the two rooms.
  vector<Point> truncated_path;
  for (const Point& node : path) {
    if (AtEdgeOfRoom(node, r2)) {
      truncated_path.clear();
    }
    truncated_path.push_back(node);
    if (AtEdgeOfRoom(node, r1)) {
      break;
    }
  }

  // Dig the corridor.
  ASSERT(truncated_path.size() > 0);
  for (const Point& node : truncated_path) {
    tiles[node.x][node.y] = Tile::FREE;
  }
  AddDoor(truncated_path[0], r2, &tiles, &diggable);
  AddDoor(truncated_path[truncated_path.size() - 1], r1, &tiles, &diggable);
}

void Level::Erode(int islandness) {
  TileArray new_tiles = tiles;
  for (int x = 1; x < size.x - 1; x++) {
    for (int y = 1; y < size.y - 1; y++) {
      if (SquareFixedByConnectivityConstraint(new_tiles, Point(x, y))) {
        continue;
      }
      const Tile tile = tiles[x][y];
      const bool blocked = IsTileBlocked(tile);
      int neighbors_blocked = 0;
      for (const Point& step : kKingMoves) {
        if (IsTileBlocked(tiles[x + step.x][y + step.y])) {
          neighbors_blocked += 1;
        }
      }
      const int matches = (blocked ? neighbors_blocked : 8 - neighbors_blocked);
      const int k = 4;
      const int l = 6;
      if (blocked) {
        new_tiles[x][y] = ((rand() % (8*k)) < 8 - matches ?
                           Tile::FREE : Tile::DEFAULT);
      } else {
        const int cutoff = max(8 - matches, matches - 8 + islandness);
        new_tiles[x][y] = ((rand() % (8*l)) < cutoff ?
                           Tile::DEFAULT : Tile::FREE);
      }
    }
  }
  tiles = new_tiles;
}

bool Level::PlaceRoom(const Room& room, int separation, vector<Room>* rooms) {
  for (const auto& other : *rooms) {
    if (RoomToRoomDistance(room, other) < separation) {
      return false;
    }
  }
  for (int x = 0; x < room.size.x; x++) {
    for (int y = 0; y < room.size.y; y++) {
      tiles[x + room.position.x][y + room.position.y] = Tile::FREE;
    }
  }
  rooms->push_back(room);
  return true;
}

string Level::ToDebugString() const {
  string result;
  for (int y = 0; y < size.y; y++) {
    string row = "\n";
    for (int x = 0; x < size.x; x++) {
      row += GetDebugCharForTile(tiles[x][y]);
    }
    result += row;
  }
  return result;
}

Point GetRandomSquareInRoom(const Room& room) {
  return room.position + Point(RandInt(0, room.size.x - 1),
                               RandInt(0, room.size.y - 1));
}

double RoomToRoomDistance(const Room& r1, const Room& r2) {
  Point distance(max(max(r1.position.x - r2.position.x - r2.size.x,
                         r2.position.x - r1.position.x - r1.size.x), 0),
                 max(max(r1.position.y - r2.position.y - r2.size.y,
                         r2.position.y - r1.position.y - r1.size.y), 0));
  return distance.length();
}

}  // namespace gen
}  // namespace babel
