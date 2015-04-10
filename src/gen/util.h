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

// rid stands for "room index".
//
// rid 0 is reserved for squares not in any room. If rids[i][j] is
// greater than 0, square (i, j) is in room rids[i][j] - 1.
typedef unsigned char rid;

struct Level {
  Level(const Point& size);

  // Turns any DEFAULT square adjacent to a FREE square into a wall.
  void AddWalls();

  // Digs a corridor between the two rooms, modifying tiles and diggable.
  //
  // Windiness is between 1.0 and 8.0, with increasing windiness causing the
  // corridor digger to take longer paths between rooms.
  void DigCorridor(const engine::TileMap::Room& r1,
                   const engine::TileMap::Room& r2, double windiness);

  // Runs an erosion step on the level. Each tile has a chance of being
  // converted to the types of the tiles around it.
  //
  // Islandness is between 0 and 16, with increasing islandness causing it to
  // be more likely to have random walls in the interior of a room.
  void Erode(int islandness);

  // Returns true and adds room to rooms if the room was successfully placed.
  bool PlaceRoom(const engine::TileMap::Room& room, int separation,
                 std::vector<engine::TileMap::Room>* rooms);

  // Returns a human-readable serialization of the level.
  std::string ToDebugString(bool show_rooms=true) const;

  const Point size;
  TileArray tiles;
  Array2d<rid> rids;
  Array2d<bool> diggable;
};

// Returns a random integer in [x, y]. NOTE: the range is inclusive!
inline int RandInt(int x, int y) {
  return (rand() % (y - x + 1)) + x;
}

// Return a uniform random square in the given room.
Point GetRandomSquareInRoom(const engine::TileMap::Room& room);

// Returns the L2 distance between the two rooms.
double RoomToRoomDistance(const engine::TileMap::Room& r1,
                          const engine::TileMap::Room& r2);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_UTIL_H__
