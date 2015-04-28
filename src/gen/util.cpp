#include "gen/util.h"

#include <algorithm>
#include <cfloat>
#include <deque>
#include <unordered_map>
#include <unordered_set>

#include "base/debug.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Graphic;
using babel::engine::Tile;
using std::deque;
using std::max;
using std::string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace babel {
namespace gen {
namespace {

const Point kBishopMoves[] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

// IMPORTANT: The king moves are arranged in increasing order of angle.
const Point kKingMoves[] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1},
                            {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

const Point kRookMoves[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

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

inline bool IsTileBlocked(Tile tile) {
  return tile == Tile::DEFAULT || tile == Tile::WALL;
}

void AddDoor(const Point& square, const Room& room,
             TileArray* tiles, Array2d<bool>* diggable) {
  for (const Point& step : kKingMoves) {
    const Point neighbor = square + step;
    if (IsTileBlocked((*tiles)[neighbor.x][neighbor.y])) {
      (*diggable)[neighbor.x][neighbor.y] = false;
    }
  }
  if (rand() % 2 == 0) {
    (*tiles)[square.x][square.y] = Tile::DOOR;
  }
}

// Returns true if it is possible to erode the given square in the map.
// A square may be immune to erosion if:
//  - It has no free orthogonal neighbors. We want all rooms to be connected
//    by rook moves, even though the player can move diagonally.
//  - It is adjacent to squares in two different rooms. Eroding it would
//    connect those two rooms, which we don't want.
//
// If this method returns true, it will set room_index to be the index of
// the adjacent free square's room.
bool CanErodeSquare(
    const Array2d<rid>& rids, const Point& square, rid* room_index) {
  *room_index = 0;
  bool has_free_orthogonal_neighbor = false;
  int min_unblocked_index = -1;
  int max_unblocked_index = -1;
  int gaps = 0;
  for (int i = 0; i < 8; i++) {
    const Point& step = kKingMoves[i];
    const rid adjacent = rids[square.x + step.x][square.y + step.y];
    if (adjacent == 0) {
      continue;
    }
    *room_index = adjacent;
    has_free_orthogonal_neighbor |= i % 2 == 0;
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
  return gaps <= 1 && has_free_orthogonal_neighbor;
}

bool HasNeighborInRoom(const Array2d<rid>& rids, const Point& square,
                       rid room_index, Point* neighbor_in_room) {
  for (const Point& step : kRookMoves) {
    *neighbor_in_room = square + step;
    if (rids[neighbor_in_room->x][neighbor_in_room->y] == room_index) {
      return true;
    }
  }
  return false;
}

}  // namespace

Level::Level(const Point& s)
    : size(s), tiles(ConstructArray2d<Tile>(s, Tile::DEFAULT)),
      rids(ConstructArray2d<rid>(s, 0)),
      diggable(ConstructArray2d<bool>(s, true)) {}

void Level::AddWalls() {
  for (int x = 0; x < size.x; x++) {
    for (int y = 0; y < size.y; y++) {
      if (IsTileBlocked(tiles[x][y])) {
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

bool Level::DigCorridor(const vector<Room>& rooms, int index1,
                        int index2, double windiness) {
  const Room& r1 = rooms[index1];
  const Room& r2 = rooms[index2];
  const Point source = r1.GetRandomSquare();
  const Point target = r2.GetRandomSquare();
  ASSERT(InBounds(source, size) && diggable[source.x][source.y]);
  ASSERT(InBounds(target, size) && diggable[target.x][target.y]);

  unordered_map<Point, double> distances{{source, 0}};
  unordered_map<Point, Point> parents;
  unordered_set<Point> visited;

  // Run Djikstra's algorithm between the source and target.
  while (distances.size() > 0 && visited.find(target) == visited.end()) {
    Point best_node;
    double best_distance = DBL_MAX;
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

  // We may have terminated without finding a path.
  if (visited.find(target) == visited.end()) {
    return false;
  }

  // Construct the actual path from source to target.
  // Guarantee that the first element of the path is target and the last source.
  Point node = target;
  vector<Point> path{node};
  while (node != source) {
    node = parents.at(node);
    path.push_back(node);
  }

  // Truncate the path to only include sections outside the two rooms.
  // Guarantee that the first element of the path is in r2 and the last in r1.
  deque<Point> truncated_path;
  for (const Point& node : path) {
    if (rids[node.x][node.y] == index2 + 1) {
      truncated_path.clear();
    }
    truncated_path.push_back(node);
    if (rids[node.x][node.y] == index1 + 1) {
      break;
    }
  }

  // Truncate the path further: remove nodes from the beginning until there
  // is exactly one vertex on the path with a rook neighbor in the second room.
  // Do the same at the end until there is exactly one vertex on the path with
  // a rook neighbor in the first room.
  ASSERT(truncated_path.size() > 2);
  Point neighbor_in_room;
  while (HasNeighborInRoom(rids, truncated_path[2],
                           index2 + 1, &neighbor_in_room)) {
    truncated_path.pop_front();
    truncated_path.pop_front();
    truncated_path.push_front(neighbor_in_room);
  }
  while (HasNeighborInRoom(rids, truncated_path[truncated_path.size() - 3],
                           index1 + 1, &neighbor_in_room)) {
    truncated_path.pop_back();
    truncated_path.pop_back();
    truncated_path.push_back(neighbor_in_room);
  }

  // Dig the corridor, but don't dig through doors.
  for (int i = 1; i < truncated_path.size() - 1; i++) {
    const Point& node = truncated_path[i];
    if (IsTileBlocked(tiles[node.x][node.y])) {
      tiles[node.x][node.y] = Tile::FREE;
    }
  }
  AddDoor(truncated_path[1], r2, &tiles, &diggable);
  AddDoor(truncated_path[truncated_path.size() - 2], r1, &tiles, &diggable);
  return true;
}

void Level::Erode(int islandness) {
  Array2d<rid> new_rids = rids;
  for (int x = 1; x < size.x - 1; x++) {
    for (int y = 1; y < size.y - 1; y++) {
      rid room_index;
      if (!CanErodeSquare(new_rids, Point(x, y), &room_index)) {
        continue;
      }
      int neighbors_blocked = 0;
      for (const Point& step : kKingMoves) {
        if (rids[x + step.x][y + step.y] == 0) {
          neighbors_blocked += 1;
        }
      }
      const bool blocked = rids[x][y] == 0;
      const int matches = (blocked ? neighbors_blocked : 8 - neighbors_blocked);
      const int inverse_blocked_to_free = 2;
      const int inverse_free_to_blocked = 4;
      const int cutoff = max(8 - matches, matches - 8 + islandness);
      const bool changed =
          (blocked ? (rand() % (8*inverse_blocked_to_free)) < 8 - matches :
                     (rand() % (8*inverse_free_to_blocked)) < cutoff);
      if (changed) {
        new_rids[x][y] = (blocked ? room_index : 0);
        tiles[x][y] = (blocked ? Tile::FREE : Tile::DEFAULT);
      }
    }
  }
  rids = new_rids;
}

void Level::ExtractFinalRooms(int n, vector<Room>* rooms) {
  rooms->clear();
  rooms->resize(n);
  for (int i = 0; i < n; i++) {
    rooms->push_back(Room());
  }
  for (int x = 0; x < size.x; x++) {
    for (int y = 0; y < size.y; y++) {
      const rid room_index = rids[x][y];
      ASSERT((room_index == 0) == IsTileBlocked(tiles[x][y]));
      if (room_index == 0) {
        continue;
      }
      ASSERT(room_index - 1 < n);
      (*rooms)[room_index - 1].squares.push_back(Point(x, y));
      for (const Point& step : kBishopMoves) {
        const Point neighbor = Point(x, y) + step;
        if (IsTileBlocked(tiles[neighbor.x][neighbor.y])) {
          bool adjacent_to_room = false;
          for (const Point& step_two : kRookMoves) {
            const Point neighbor_two = neighbor + step_two;
            if (InBounds(neighbor_two, size) &&
                room_index == rids[neighbor_two.x][neighbor_two.y]) {
              adjacent_to_room = true;
            }
          }
          if (!adjacent_to_room) {
            diggable[neighbor.x][neighbor.y] = false;
          }
        }
      }
    }
  }
}

bool Level::PlaceRectangularRoom(
    const Rect& rect, int separation, vector<Rect>* rects) {
  for (const auto& other : *rects) {
    if (RectToRectDistance(rect, other) < separation) {
      return false;
    }
  }
  const rid room_index = rects->size() + 1;
  for (int x = 0; x < rect.size.x; x++) {
    for (int y = 0; y < rect.size.y; y++) {
      tiles[x + rect.position.x][y + rect.position.y] = Tile::FREE;
      rids[x + rect.position.x][y + rect.position.y] = room_index;
    }
  }
  rects->push_back(rect);
  return true;
}

string Level::ToDebugString(bool show_rooms) const {
  string result;
  for (int y = 0; y < size.y; y++) {
    string row = "\n";
    for (int x = 0; x < size.x; x++) {
      row += GetDebugCharForTile(tiles[x][y]);
      if (show_rooms && rids[x][y] > 0) {
        row[row.size() - 1] = char(int('0') + (rids[x][y] - 1) % 10);
      }
    }
    result += row;
  }
  return result;
}

double RectToRectDistance(const Rect& r1, const Rect& r2) {
  Point distance(max(max(r1.position.x - r2.position.x - r2.size.x,
                         r2.position.x - r1.position.x - r1.size.x), 0),
                 max(max(r1.position.y - r2.position.y - r2.size.y,
                         r2.position.y - r1.position.y - r1.size.y), 0));
  return distance.length();
}

}  // namespace gen
}  // namespace babel
