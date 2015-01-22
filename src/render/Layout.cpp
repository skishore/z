#include <algorithm>
#include <vector>

#include "base/debug.h"
#include "render/Layout.h"

using std::map;
using std::max;
using std::min;
using std::vector;

namespace babel {
namespace render {
namespace {

inline int sign(int x) {
  return (x > 0 ? 1 : (x < 0 ? -1 : 0));
}

struct BasicRect {
  int x;
  int y;
  int w;
  int h;
};

// Returns the area of the intersection of the two rects.
int Intersection(const BasicRect& rect1, const BasicRect& rect2) {
  const Point lub(max(rect1.x, rect2.x), max(rect1.y, rect2.y));
  const Point glb(min(rect1.x + rect1.w, rect2.x + rect2.w),
                  min(rect1.y + rect1.h, rect2.y + rect2.h));
  return max(glb.x - lub.x, 0)*max(glb.y - lub.y, 0);
}

Point PositionRect(int index, const Point& position, BasicRect* rect) {
  const int grid_size = rect->h;
  if (index < 6) {
    int y_offset = (index % 3) - 1;
    rect->x = (index < 3 ? position.x - rect->w : position.x + grid_size);
    if (y_offset != 0) {
      rect->x += (index < 3 ? 1 : -1)*grid_size;
    }
    rect->y = y_offset*grid_size + position.y;
    return Point((index < 3 ? -1 : 1), y_offset);
  }
  rect->x = position.x + (grid_size - rect->w)/2;
  rect->y = position.y + (2*(index % 2) - 1)*grid_size;
  return Point(0, 2*(index % 2) - 1);
}

int GetScore(const Point& direction, const Point& discriminant,
             const BasicRect& rect, const vector<BasicRect>& sprite_rects,
             const vector<BasicRect>& text_rects) {
  int score = 0;
  if (direction.x == discriminant.x) {
    score += 1;
  }
  if (direction.y == discriminant.y) {
    score += 1;
  }
  if (direction.y == 0) {
    score += 4;
  }
  for (const auto& sprite_rect : sprite_rects) {
    score -= Intersection(rect, sprite_rect);
  }
  for (const auto& text_rect : text_rects) {
    score -= Intersection(rect, text_rect);
  }
  return score;
}

Point GetBestDirection(
    int grid_size, const Point& dimensions, const Point& position, int size,
    const vector<BasicRect>& sprite_rects, vector<BasicRect>* text_rects) {
  BasicRect rect{0, 0, grid_size*(min(size, 4) + 2)/2, grid_size};
  Point discriminant = 2*position + Point(grid_size, grid_size) - dimensions;
  discriminant.x = sign(discriminant.x);
  discriminant.y = sign(discriminant.y);

  int best_score = INT_MIN;
  int best_index = 0;
  Point best_direction;
  for (int i = 0; i < 8; i++) {
    Point direction = PositionRect(i, position, &rect);
    int score = GetScore(
        direction, discriminant, rect, sprite_rects, *text_rects);
    if (score > best_score) {
      best_score = score;
      best_index = i;
      best_direction = direction;
    }
  }
  PositionRect(best_index, position, &rect);
  text_rects->push_back(rect);
  return best_direction;
}

}  // namespace

Layout::Layout(int grid_size, const Point& dimensions)
    : grid_size_(grid_size), dimensions_(dimensions) {}

map<engine::sid,Point> Layout::Place(
    const engine::View& view, const map<engine::sid,Point>& sprite_positions) {
  map<engine::sid,Point> result;

  vector<BasicRect> sprite_rects;
  vector<BasicRect> text_rects;
  for (const auto& pair : sprite_positions) {
    const Point& point = pair.second;
    sprite_rects.push_back(BasicRect{point.x, point.y, grid_size_, grid_size_});
  }

  for (const auto& pair : sprite_positions) {
    const engine::SpriteView& sprite = view.sprites.at(pair.first);
    if (sprite.text.empty()) {
      continue;
    }
    result[pair.first] = GetBestDirection(
        grid_size_, dimensions_, pair.second, sprite.text.size(),
        sprite_rects, &text_rects);
  }
  return result;
}

} // namespace render
} // namespace babel
