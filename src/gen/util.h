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

struct Rect {
  Point size;
  Point position;
};

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
  // Returns true if the corridor was successfully dug.
  //
  // Windiness is between 1.0 and 8.0, with increasing windiness causing the
  // corridor digger to take longer paths between rooms.
  bool DigCorridor(const std::vector<engine::TileMap::Room>& rooms,
                   int index1, int index2, double windiness);

  // Runs an erosion step on the level. Each tile has a chance of being
  // converted to the types of the tiles around it.
  //
  // Islandness is between 0 and 16, with increasing islandness causing it to
  // be more likely to have random walls in the interior of a room.
  void Erode(int islandness);

  // Fills the rooms array with the final (non-rectangular) rooms.
  void ExtractFinalRooms(int n, std::vector<engine::TileMap::Room>* rooms);

  // Returns true and adds rect to rects if the room was successfully placed.
  bool PlaceRectangularRoom(const Rect& rect, int separation,
                            std::vector<Rect>* rects);

  // Returns a human-readable serialization of the level.
  std::string ToDebugString(bool show_rooms=false) const;

  const Point size;
  TileArray tiles;
  Array2d<rid> rids;
  Array2d<bool> diggable;
};

// Returns a random integer in [x, y]. NOTE: the range is inclusive!
inline int RandInt(int x, int y) {
  return (rand() % (y - x + 1)) + x;
}

// Returns the L2 distance between the two rooms.
double RectToRectDistance(const Rect& r1, const Rect& r2);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_UTIL_H__
