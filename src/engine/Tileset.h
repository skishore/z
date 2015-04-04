#ifndef __BABEL_ENGINE_TILESET_H__
#define __BABEL_ENGINE_TILESET_H__

#include "base/point.h"

namespace babel {
namespace engine {

typedef unsigned char Graphic;

enum Tile {
  DEFAULT = 0,
  FREE = 1,
  WALL = 2,
  DOOR = 3,
  FENCE = 4
};

class Tileset {
 public:
  // This function is NOT necessarily deterministic, so when a map is generated,
  // a graphic should be saved for each tile in the map.
  virtual Graphic GetGraphicForTile(Tile tile) const = 0;
};

} // namespace engine
} // namespace babel

#endif  // __BABEL_ENGINE_TILE_MAP_H__
