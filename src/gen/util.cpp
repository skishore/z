#include "gen/util.h"

#include <algorithm>

#include "base/debug.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Tile;
using babel::engine::Tileset;
using std::max;
using std::string;
using std::vector;

namespace babel {
namespace gen {
namespace {

inline string GetDebugCharForCell(Cell cell) {
  if (cell == Cell::FREE) {
    return ".";
  } else if (cell == Cell::BLOCKED) {
    return "#";
  }
  return " ";
}

inline Tile GetTileForCell(const Tileset& tileset, Cell cell) {
  if (cell == Cell::FREE) {
    return tileset.GetFreeTile();
  } else if (cell == Cell::BLOCKED) {
    return tileset.GetBlockedTile();
  }
  return tileset.default_tile;
}

}  // namespace

bool PlaceRoom(const Room& room, int separation, CellArray* cells,
               Array2d<bool>* diggable, vector<Room>* rooms) {
  for (const auto& other : *rooms) {
    if (RoomToRoomDistance(room, other) < separation) {
      return false;
    }
  }
  for (int x = 0; x < room.size.x; x++) {
    for (int y = 0; y < room.size.y; y++) {
      (*cells)[x + room.position.x][y + room.position.y] = Cell::FREE;
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

string ComputeDebugString(const CellArray& cells) {
  string result;
  for (int x = 0; x < cells.size(); x++) {
    string row = "\n";
    for (int y = 0; y < cells.size(); y++) {
      row += GetDebugCharForCell(cells[x][y]);
    }
    result += row;
  }
  return result;
}

Tile* ComputeTiles(const Tileset& tileset, const CellArray& cells) {
  ASSERT(cells.size() > 0, "Got empty cell array.");
  const int height = cells[0].size();
  Tile* tiles = new Tile[cells.size()*cells[0].size()];
  for (int x = 0; x < cells.size(); x++) {
    ASSERT(cells[x].size() == height, "Got non-rectangular cell array.");
    for (int y = 0; y < height; y++) {
      tiles[x*height + y] = GetTileForCell(tileset, cells[x][y]);
    }
  }
  return tiles;
}

}  // namespace gen
}  // namespace babel
