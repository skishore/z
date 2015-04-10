#include "engine/TileMap.h"

using std::string;
using std::vector;

namespace babel {
namespace engine {

Point TileMap::Room::GetRandomSquare() const {
  return squares[rand() % squares.size()];
}

Graphic TileMap::GetGraphic(const Point& square) const {
  if (0 <= square.x && square.x < size_.x &&
      0 <= square.y && square.y < size_.y) {
    return graphics_[square.x*size_.y + square.y];
  }
  return tileset_->GetGraphicForTile(Tile::DEFAULT);
}

Tile TileMap::GetTile(const Point& square) const {
  if (0 <= square.x && square.x < size_.x &&
      0 <= square.y && square.y < size_.y) {
    return tiles_[square.x*size_.y + square.y];
  }
  return Tile::DEFAULT;
}

bool TileMap::IsSquareBlocked(const Point& square) const {
  return GetTile(square) != Tile::FREE;
}

void TileMap::SetTile(const Point& square, Tile tile) {
  if (0 <= square.x && square.x < size_.x &&
      0 <= square.y && square.y < size_.y) {
    const int index = square.x*size_.y + square.y;
    if (tiles_[index] != tile) {
      graphics_[index] = tileset_->GetGraphicForTile(tile);
      tiles_[index] = tile;
    }
  }
}

void TileMap::PackTiles(const vector<vector<Tile>>& tiles) {
  const int size = size_.x*size_.y;
  ASSERT(size > 0);
  graphics_.reset(new Graphic[size]);
  tiles_.reset(new Tile[size]);
  ASSERT(tiles.size() == size_.x);
  for (int x = 0; x < size_.x; x++) {
    ASSERT(tiles[x].size() == size_.y);
    for (int y = 0; y < size_.y; y++) {
      const Tile tile = tiles[x][y];
      const int index = x*size_.y + y;
      graphics_[index] = tileset_->GetGraphicForTile(tile);
      tiles_[index] = tile;
    }
  }
}

}  // namespace engine
} // namespace babel
