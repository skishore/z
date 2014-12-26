#ifndef __BABEL_TILE_MAP_H__
#define __BABEL_TILE_MAP_H__

#include <iostream>
#include <string>
#include <vector>

#include "Point.h"

namespace babel {

typedef unsigned char Tile;

class TileMap {
 public:
  struct Room {
    bool Contains(const Point& square) const;

    Point position;
    Point size;
  };

  void LoadMap(const std::string& filename);
  Tile GetMapTile(const Point& square) const;
  bool IsSquareBlocked(const Point& square) const;

  const std::vector<Room>& GetRooms() const { return rooms_; }
  const Point& GetSize() const { return map_dimensions_; };
  const Point& GetStartingSquare() const { return starting_square_; }

 private:
  // Information about the whole map: its dimensions, its packed 1d tile array,
  // and its default tile (returned when a point outside the map is accessed).
  Point map_dimensions_;
  std::unique_ptr<Tile[]> map_tiles_;
  Tile map_default_tile_;
  Point starting_square_;
  std::vector<Room> rooms_;
};

} // namespace babel

#endif  // __BABEL_TILE_MAP_H__
