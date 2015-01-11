#ifndef __BABEL_TRANSFORM_H__
#define __BABEL_TRANSFORM_H__

#include <map>
#include <set>
#include <unordered_map>

#include "base/point.h"
#include "engine/Sprite.h"

namespace babel {
namespace ui {

static const int kGridSize = 32;

struct Transform {
  struct Shade {
    Uint32 color;
    float alpha;
  };

  Point camera_offset;
  std::map<engine::sid,Point> sprite_offsets;
  std::set<engine::sid> hidden_sprites;
  std::unordered_map<Point,Shade> shaded_squares;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_TRANSFORM_H__
