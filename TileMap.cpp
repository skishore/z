#include <fstream>
#include <iostream>

#include "TileMap.h"

using std::string;
using std::vector;

namespace skishore {

TileMap::TileMap(const Point& zone_size) :
    zone_size_(zone_size),
    zone_tiles_(zone_size.x, vector<Tile>(zone_size.y, '\0')) {};

bool TileMap::LoadMap(const string& filename) {
  std::ifstream file("data/" + filename, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    return false;
  }

  // Read out the map's height and width.
  string skip_token;
  file >> skip_token >> map_dimensions_.x >> skip_token >> map_dimensions_.y;
  const int map_size = map_dimensions_.x*map_dimensions_.y;

  // Read out the default tile.
  int map_default_tile;
  file >> skip_token >> map_default_tile;
  map_default_tile_ = map_default_tile;

  // Read out the actual map data.
  file >> skip_token;
  file.seekg(1, std::ios::cur);
  map_tiles_.reset(new Tile[map_size]);
  file.read((char*)map_tiles_.get(), map_size);
  if (file.gcount() < map_size) {
    return false;
  }

  return true;
}

Tile TileMap::GetTile(const Point& point) const {
  if (0 <= point.x && point.x < map_dimensions_.x &&
      0 <= point.y && point.y < map_dimensions_.y) {
    return map_tiles_[point.x*map_dimensions_.y + point.y];
  }
  return map_default_tile_;
}

} // namespace skishore
