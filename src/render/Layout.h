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
  std::map<engine::sid,Point> Place(
      const engine::View& view,
      const std::map<engine::sid,Point>& sprite_positions);
};

} // namespace render
} // namespace babel

#endif  // __BABEL_LAYOUT_H__
