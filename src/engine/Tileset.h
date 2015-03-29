#ifndef __BABEL_ENGINE_TILESET_H__
#define __BABEL_ENGINE_TILESET_H__

#include "base/point.h"

namespace babel {
namespace engine {

typedef unsigned char Tile;

struct Tileset {
  // For now, we assume that all free tiles come before all blocked tiles.
  const Tile blocked_tile;
  const Tile default_tile;

  inline bool IsTileBlocked(Tile tile) const {
    return tile >= blocked_tile;
  };

  inline Tile GetBlockedTile() const {
    return blocked_tile;
  };

  inline Tile GetFreeTile() const {
    return rand() % blocked_tile;
  };
};

} // namespace engine
} // namespace babel

#endif  // __BABEL_ENGINE_TILE_MAP_H__
