#ifndef __BABEL_ENGINE_TILE_MAP_H__
#define __BABEL_ENGINE_TILE_MAP_H__

#include <string>
#include <vector>

#include "base/point.h"
#include "engine/tileset.h"

namespace babel {
namespace engine {

class TileMap {
 public:
  struct Room {
    Point GetRandomSquare() const;
    std::vector<Point> squares;
  };

  Graphic GetGraphic(const Point& square) const;
  Tile GetTile(const Point& square) const;
  bool IsSquareBlocked(const Point& square) const;

  const std::vector<Room>& GetRooms() const { return rooms_; }
  const Point& GetSize() const { return size_; };
  const Point& GetStartingSquare() const { return starting_square_; }

  void SetTile(const Point& square, Tile tile);

 protected:
  TileMap() {};

  // Uses the given tile vector to set graphics_ and tiles_.
  void PackTiles(const std::vector<std::vector<Tile>>& tiles);

  // Information about the whole map: its dimensions, its packed 1d tile array,
  // and its default tile (returned when a point outside the map is accessed).
  //
  // Subclasses of TileMap correspond to different level generation algorithms.
  // These members are protected so that levelgen can edit them.
  Point size_;
  std::unique_ptr<Graphic[]> graphics_;
  std::unique_ptr<Tile[]> tiles_;
  std::unique_ptr<Tileset> tileset_;
  Point starting_square_;
  std::vector<Room> rooms_;
};

} // namespace engine
} // namespace babel

#endif  // __BABEL_ENGINE_TILE_MAP_H__
