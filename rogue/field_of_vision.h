#ifndef ROGUE_FIELD_OF_VISION_H__
#define ROGUE_FIELD_OF_VISION_H__

#include <vector>

#include "point.h"
#include "tile_map.h"

class FieldOfVision {
 public:
  // Computes field-of-vision from the given a tile map and a source point.
  FieldOfVision(const TileMap& tiles, const Point& source);

  bool IsSquareVisible(int x, int y) const;

  // Interface methods needed to use this class with the permissive-fov library.
  bool isBlocked(int x, int y) const;
  void visit(int x, int y);

 private:
  const TileMap& tiles_;
  const Point& source_;
  std::vector<std::vector<bool>> is_square_visible_;
};

#endif  // ROGUE_FIELD_OF_VISION_H__
