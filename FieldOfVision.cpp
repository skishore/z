#include "FieldOfVision.h"

#include <iostream>

#include "permissive-fov.cc"
#include "permissive-fov-cpp.h"

#include "debug.h"

using std::vector;

namespace babel {

FieldOfVision::FieldOfVision(
    const TileMap& map, const Point& source, int bound)
    : map_(map), source_(source), offset_(source - Point(bound, bound)),
      size_(2*bound + 1), is_square_visible_(size_, vector<bool>(size_)) {
  permissive::squareFov<FieldOfVision>(source_.x, source_.y, bound, *this);
}

bool FieldOfVision::IsSquareVisible(const Point& square, int radius) const {
  Point offset_square = square - offset_;
  if (0 <= offset_square.x && offset_square.x < size_ &&
      0 <= offset_square.y && offset_square.y < size_) {
    return (is_square_visible_[offset_square.x][offset_square.y] &&
            (square - source_).length() < radius);
  }
  return false;
}

bool FieldOfVision::isBlocked(int x, int y) const {
  return map_.IsSquareBlocked(Point(x, y));
}

void FieldOfVision::visit(int x, int y) {
  // Visibility should never extend outside more than one square outside the
  // field of vision's bounds.
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
