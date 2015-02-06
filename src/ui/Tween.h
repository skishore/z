#ifndef __BABEL_UI_TWEEN_H__
#define __BABEL_UI_TWEEN_H__

#include <map>
#include <memory>
#include <vector>

#include "base/point.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "render/Graphics.h"
#include "render/Transform.h"

namespace babel {
namespace ui {

class TweenEvent {
 public:
  virtual void Update(int frame, render::Transform* transform) = 0;
};

class Tween {
 public:
  Tween(const engine::View& start, const engine::View& end);

  // Update returns false if the tween is complete.
  bool Update();
  void Draw(render::Graphics* graphics) const;

 // Methods below this point are private.
 // TODO(skishore): Figure out a way to encapsulate them.
  void AddEvent(TweenEvent* event);

  const engine::View& start;
  const engine::View& end;

  int frame = 0;
  render::Transform transform;
  std::vector<std::unique_ptr<TweenEvent>> events;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_UI_TWEEN_H__
