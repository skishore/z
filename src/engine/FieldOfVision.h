#ifndef BABEL_FIELD_OF_VISION_H__
#define BABEL_FIELD_OF_VISION_H__

#include <vector>

#include "base/Point.h"
#include "engine/TileMap.h"

namespace babel {
namespace engine {

class FieldOfVision {
 public:
  // Computes field-of-vision from the given a tile map and a source point.
  // Squares that are more than bound away in x- or y-coordinate are hidden.
  FieldOfVision(const TileMap& tiles, const Point& source, int bound);

  bool IsSquareVisible(const Point& square, int radius) const;

  // Interface methods needed to use this class with the permissive-fov library.
  bool isBlocked(int x, int y) const;
  void visit(int x, int y);

 private:
  const TileMap& map_;
  const Point source_;
  const Point offset_;
  const int size_;
  std::vector<std::vector<bool>> is_square_visible_;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_FIELD_OF_VISION_H__
