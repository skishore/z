#ifndef __BABEL_LAYOUT_H__
#define __BABEL_LAYOUT_H__

#include <map>

#include "base/point.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "render/Transform.h"

namespace babel {
namespace render {

class Layout {
 public:
  Layout(int grid_size, const Point& center);

  std::map<engine::sid,Point> Place(
      const engine::View& view,
      const std::map<engine::sid,Point>& sprite_positions);

 private:
  const int grid_size_;
  const Point dimensions_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_LAYOUT_H__
