#include "FieldOfVision.h"

#include <iostream>

#include "permissive-fov.cc"
#include "permissive-fov-cpp.h"

#include "debug.h"

namespace skishore {

FieldOfVision::FieldOfVision(const TileMap& tiles, const Point& source)
    : tiles_(tiles), source_(source), is_square_visible_(tiles.cols) {
  for (int x = 0; x < tiles_.cols; x++) {
    for (int y = 0; y < tiles_.rows; y++) {
      is_square_visible_[x].push_back(false);
    }
  }
  permissive::squareFov<FieldOfVision>(
      source_.x, source_.y, tiles_.cols + tiles_.rows, *this);
}

bool FieldOfVision::IsSquareVisible(int x, int y) const {
  return is_square_visible_[x][y];
}

bool FieldOfVision::isBlocked(int x, int y) const {
  return tiles_.IsSquareBlocked(x, y);
}

void FieldOfVision::visit(int x, int y) {
  // Visibility should never extend outside more than one square outside the
  // viewport rectangle.
  ASSERT(-1 <= x && x <= tiles_.cols && -1 <= y && y <= tiles_.rows,
         "Out of bounds durin visibility calculation: " << x << " " << y);
  if (0 <= x && x < tiles_.cols && 0 <= y && y < tiles_.rows) {
    is_square_visible_[x][y] = true;
  }
}

}  // namespace skishore
