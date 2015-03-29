#include "engine/TileMap.h"

using std::string;
using std::vector;

namespace babel {
namespace engine {

Tile TileMap::GetMapTile(const Point& square) const {
  if (0 <= square.x && square.x < size_.x &&
      0 <= square.y && square.y < size_.y) {
    return tiles_[square.x*size_.y + square.y];
  }
  return tileset_->default_tile;
}

bool TileMap::IsSquareBlocked(const Point& square) const {
  return tileset_->IsTileBlocked(GetMapTile(square));
}

}  // namespace engine
} // namespace babel
