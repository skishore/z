#ifndef ROGUE_FIELD_OF_VISION_H__
#define ROGUE_FIELD_OF_VISION_H__

#include <vector>

#include "Point.h"
#include "TileMap.h"

namespace skishore {

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

}  // namespace skishore

#endif  // ROGUE_FIELD_OF_VISION_H__
