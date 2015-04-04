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
const Point kKingMoves[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1},
                            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

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

}  // namespace

void AddWalls(const Point& size, TileArray* tiles) {
  for (int x = 0; x < size.x; x++) {
    for (int y = 0; y < size.y; y++) {
      if ((*tiles)[x][y] != Tile::FREE) {
        continue;
      }
      for (const Point& step : kKingMoves) {
        const Point square(x + step.x, y + step.y);
        if (0 <= square.x && square.x < size.x &&
            0 <= square.y && square.y < size.y &&
            (*tiles)[square.x][square.y] == Tile::DEFAULT) {
          (*tiles)[square.x][square.y] = Tile::WALL;
        }
      }
    }
  }
}

void DigCorridor(const Room& r1, const Room& r2, const Point& size,
                 double windiness, TileArray* tiles, Array2d<bool>* diggable) {
  const Point source = GetRandomSquareInRoom(r1);
  const Point target = GetRandomSquareInRoom(r2);
  ASSERT(InBounds(source, size) && (*diggable)[source.x][source.y]);
  ASSERT(InBounds(target, size) && (*diggable)[target.x][target.y]);

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
      if (!(InBounds(child, size) && (*diggable)[child.x][child.y] &&
            visited.find(child) == visited.end())) {
        continue;
      }
      const Tile tile = (*tiles)[child.x][child.y];
      const bool free = (tile == Tile::FREE || tile == Tile::DOOR);
      const double distance = best_distance + (free ? windiness : 2.0);
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
    (*tiles)[node.x][node.y] = Tile::FREE;
  }
  AddDoor(truncated_path[0], r2, tiles, diggable);
  AddDoor(truncated_path[truncated_path.size() - 1], r1, tiles, diggable);
}

Point GetRandomSquareInRoom(const Room& room) {
  return room.position + Point(RandInt(0, room.size.x - 1),
                               RandInt(0, room.size.y - 1));
}

bool PlaceRoom(const Room& room, int separation, TileArray* tiles,
               Array2d<bool>* diggable, vector<Room>* rooms) {
  for (const auto& other : *rooms) {
    if (RoomToRoomDistance(room, other) < separation) {
      return false;
    }
  }
  for (int x = 0; x < room.size.x; x++) {
    for (int y = 0; y < room.size.y; y++) {
      (*tiles)[x + room.position.x][y + room.position.y] = Tile::FREE;
    }
  }
  (*diggable)[room.position.x - 1][room.position.y - 1] = false;
  (*diggable)[room.position.x - 1][room.position.y + room.size.y] = false;
  (*diggable)[room.position.x + room.size.x][room.position.y - 1] = false;
  (*diggable)[room.position.x+room.size.x][room.position.y+room.size.y] = false;
  rooms->push_back(room);
  return true;
}

double RoomToRoomDistance(const Room& r1, const Room& r2) {
  Point distance(max(max(r1.position.x - r2.position.x - r2.size.x,
                         r2.position.x - r1.position.x - r1.size.x), 0),
                 max(max(r1.position.y - r2.position.y - r2.size.y,
                         r2.position.y - r1.position.y - r1.size.y), 0));
  return distance.length();
}

string ComputeDebugString(const TileArray& tiles) {
  string result;
  for (int x = 0; x < tiles.size(); x++) {
    string row = "\n";
    for (int y = 0; y < tiles.size(); y++) {
      row += GetDebugCharForTile(tiles[x][y]);
    }
    result += row;
  }
  return result;
}

}  // namespace gen
}  // namespace babel
