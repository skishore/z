#ifndef __BABEL_GEN_UTIL_H__
#define __BABEL_GEN_UTIL_H__

#include <vector>

#include "base/point.h"
#include "engine/Tileset.h"
#include "engine/TileMap.h"

namespace babel {
namespace gen {

template<typename T>
using Array2d = std::vector<std::vector<T>>;

enum Cell {
  FREE = 0,
  BLOCKED = 1,
  DEFAULT = 2
};

typedef Array2d<Cell> CellArray;

template<typename T>
Array2d<T> ConstructArray2d(const Point& size, T value) {
  return Array2d<T>(size.x, std::vector<T>(size.y, value));
}

// Returns a random integer in [x, y]. NOTE: the range is inclusive!
inline int RandInt(int x, int y) {
  return (rand() % (y - x + 1)) + x;
}

// Returns true and adds room to rooms if the room was successfully placed.
bool PlaceRoom(const engine::TileMap::Room& room, int separation,
               CellArray* cells, Array2d<bool>* diggable,
               std::vector<engine::TileMap::Room>* rooms_);

// Returns the L2 distance between the two rooms.
double RoomToRoomDistance(const engine::TileMap::Room& r1,
                          const engine::TileMap::Room& r2);

// Converts a 2d array of standardized cells into a human-readable string.
std::string ComputeDebugString(const CellArray& blocked);

// Converts a 2d array of standardized cells into actual tiles, given a tileset.
engine::Tile* ComputeTiles(
    const engine::Tileset& tileset, const CellArray& blocked);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_UTIL_H__
