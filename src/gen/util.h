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

typedef Array2d<engine::Tile> TileArray;

template<typename T>
Array2d<T> ConstructArray2d(const Point& size, T value) {
  return Array2d<T>(size.x, std::vector<T>(size.y, value));
}

// Returns a random integer in [x, y]. NOTE: the range is inclusive!
inline int RandInt(int x, int y) {
  return (rand() % (y - x + 1)) + x;
}

// Turns any DEFAULT square adjacent to a FREE square into a wall.
void AddWalls(const Point& size, TileArray* tiles);

// Digs a corridor between the two rooms, modifying tiles and diggable.
//
// Windiness is between 1.0 and 8.0, with increasing windiness causing the
// corridor digger to take longer paths between rooms.
void DigCorridor(const engine::TileMap::Room& r1,
                 const engine::TileMap::Room& r2, const Point& size,
                 double windiness, TileArray* tiles, Array2d<bool>* diggable);

// Return a uniform random square in the given room.
Point GetRandomSquareInRoom(const engine::TileMap::Room& room);

// Returns true and adds room to rooms if the room was successfully placed.
bool PlaceRoom(const engine::TileMap::Room& room, int separation,
               TileArray* tiles, Array2d<bool>* diggable,
               std::vector<engine::TileMap::Room>* rooms_);

// Returns the L2 distance between the two rooms.
double RoomToRoomDistance(const engine::TileMap::Room& r1,
                          const engine::TileMap::Room& r2);

// Converts a 2d array of tiles into a human-readable string.
std::string ComputeDebugString(const TileArray& blocked);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_UTIL_H__
