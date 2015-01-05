#ifndef __BABEL_ANIMATION_H__
#define __BABEL_ANIMATION_H__

#include <memory>

#include "base/point.h"
#include "engine/EventHandler.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "ui/Graphics.h"

namespace babel {
namespace ui {

class Tween;

class Animation : public engine::EventHandler {
 public:
  Animation(const engine::GameState& game_state);
  ~Animation();

  // This class takes ownership of the new view.
  void SetNextView(engine::View* view);

  // Returns true if the animation is complete.
  bool Update();

 private:
  void Commit();
  void Draw();
  void Reset();

  Graphics graphics_;
  std::unique_ptr<engine::View> last_;
  std::unique_ptr<engine::View> next_;
  const engine::GameState& game_state_;

  Tween* tween_ = nullptr;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_ANIMATION_H__
