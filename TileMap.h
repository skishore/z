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
  TileMap(const Point& zone_size);

  // Fills in map / zone data from a file / offset within the map.
  bool LoadMap(const std::string& filename);
  void LoadZone(const Point& zone_offset);

  std::ostream& PrintDebug(std::ostream& out) const;

 private:
  // Returns the tile at the given map coordinates.
  Tile GetMapTile(const Point& point) const;

  // Information about the whole map: its dimensions, its packed 1d tile array,
  // and its default tile (returned when a point outside the map is accessed).
  Point map_dimensions_;
  std::unique_ptr<Tile[]> map_tiles_;
  Tile map_default_tile_;

  // Information about the current zone: its size, its offset within the map,
  // and the 2d array of tiles within it.
  Point zone_size_;
  Point zone_offset_;
  std::vector<std::vector<Tile> > zone_tiles_;
};

inline std::ostream& operator<<(std::ostream& out, const TileMap& tile_map) {
  return tile_map.PrintDebug(out);
}

} // namespace skishore

#endif  // __SKISHORE_TILE_MAP_H__
