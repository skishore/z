#ifndef __SKISHORE_TILE_MAP_H__
#define __SKISHORE_TILE_MAP_H__

#include <iostream>
#include <string>
#include <vector>

#include "Point.h"

namespace skishore {

typedef unsigned char Tile;

class TileMap {
 public:
  bool LoadMap(const std::string& filename);
  Tile GetMapTile(const Point& point) const;

 private:
  // Information about the whole map: its dimensions, its packed 1d tile array,
  // and its default tile (returned when a point outside the map is accessed).
  Point map_dimensions_;
  std::unique_ptr<Tile[]> map_tiles_;
  Tile map_default_tile_;
};

} // namespace skishore

#endif  // __SKISHORE_TILE_MAP_H__
