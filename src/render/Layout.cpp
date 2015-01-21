#include "render/Layout.h"

using std::map;

namespace babel {
namespace render {

Layout::Layout(int grid_size, const Point& dimensions)
    : grid_size_(grid_size), dimensions_(dimensions) {}

map<engine::sid,Point> Layout::Place(
    const engine::View& view, const map<engine::sid,Point>& sprite_positions) {
  map<engine::sid,Point> result;
  for (const auto& pair : sprite_positions) {
    const engine::SpriteView& sprite = view.sprites.at(pair.first);
    if (sprite.text.empty()) {
      continue;
    }
    const Point discriminant = 2*pair.second + Point(grid_size_, grid_size_);
    Point dir;
    if (discriminant.x == dimensions_.x) {
      dir.y = (discriminant.y > dimensions_.y ? 1 : -1);
    } else {
      dir.x = (discriminant.x > dimensions_.x ? 1 : -1);
    }
    result[pair.first] = dir;
  }
  return result;
}

} // namespace render
} // namespace babel
