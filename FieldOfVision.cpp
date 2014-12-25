#include "FieldOfVision.h"

#include <iostream>

#include "permissive-fov.cc"
#include "permissive-fov-cpp.h"

#include "debug.h"

using std::vector;

namespace babel {

FieldOfVision::FieldOfVision(
    const TileMap& map, const Point& source, int radius)
    : map_(map), offset_(source - Point(radius, radius)),
      size_(2*radius + 1), is_square_visible_(size_, vector<bool>(size_)) {
  permissive::squareFov<FieldOfVision>(
      offset_.x + radius, offset_.y + radius, radius, *this);
}

bool FieldOfVision::IsSquareVisible(const Point& square) const {
  Point offset_square = square - offset_;
  if (0 <= offset_square.x && offset_square.x < size_ &&
      0 <= offset_square.y && offset_square.y < size_) {
    return is_square_visible_[offset_square.x][offset_square.y];
  }
  return false;
}

bool FieldOfVision::isBlocked(int x, int y) const {
  return map_.IsSquareBlocked(Point(x, y));
}

void FieldOfVision::visit(int x, int y) {
  // Visibility should never extend outside more than one square outside the
  // requested radius.
  Point offset_square(x - offset_.x, y - offset_.y);
  ASSERT(-1 <= offset_square.x && offset_square.x <= size_ &&
         -1 <= offset_square.y && offset_square.y <= size_,
         "Out of bounds during visibility calculation: " << offset_square);
  if (0 <= offset_square.x && offset_square.x < size_ &&
      0 <= offset_square.y && offset_square.y < size_) {
    is_square_visible_[offset_square.x][offset_square.y] = true;
  }
}

}  // namespace babel
