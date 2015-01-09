#ifndef __BABEL_TWEEN_H__
#define __BABEL_TWEEN_H__

#include <map>
#include <memory>
#include <vector>

#include "base/point.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "ui/Graphics.h"

namespace babel {
namespace ui {

class Tween;

class TweenEvent {
 public:
  virtual void Update(Tween* tween) = 0;
};

class Tween {
 public:
  Tween(const engine::View& start, const engine::View& end);

  // Update returns false if the tween is complete.
  bool Update();
  void Draw(Graphics* graphics) const;

 // Methods below this point are private.
 // TODO(skishore): Figure out a way to encapsulate them.
  void AddEvent(TweenEvent* event);

  const engine::View& start;
  const engine::View& end;

  int frame = 0;
  std::vector<std::unique_ptr<TweenEvent>> events;
  Point camera_offset;
  std::map<engine::sid,Point> sprite_offsets;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_TWEEN_H__
