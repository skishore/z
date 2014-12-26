#ifndef BABEL_FIELD_OF_VISION_H__
#define BABEL_FIELD_OF_VISION_H__

#include <vector>

#include "Point.h"
#include "TileMap.h"

namespace babel {

class FieldOfVision {
 public:
  // Computes field-of-vision from the given a tile map and a source point.
  FieldOfVision(const TileMap& tiles, const Point& source, int radius);

  bool IsSquareVisible(const Point& square) const;

  // Interface methods needed to use this class with the permissive-fov library.
  bool isBlocked(int x, int y) const;
  void visit(int x, int y);

 private:
  const TileMap& map_;
  const Point offset_;
  const int size_;
  std::vector<std::vector<bool>> is_square_visible_;
};

}  // namespace babel

#endif  // BABEL_FIELD_OF_VISION_H__