#ifndef __BABEL_ANIMATION_H__
#define __BABEL_ANIMATION_H__

#include <memory>

#include "engine/EventHandler.h"
#include "engine/View.h"
#include "ui/Graphics.h"
#include "ui/Tween.h"

namespace babel {
namespace ui {

class Animation : public engine::EventHandler {
 public:
  Animation(const engine::GameState& game_state);

  void Checkpoint();
  void Draw();

  // Returns true if the current animation is complete.
  bool Update();

 private:
  void Commit();

  const engine::GameState& game_state_;

  Graphics graphics_;
  std::unique_ptr<engine::View> last_;
  std::unique_ptr<engine::View> next_;
  std::unique_ptr<Tween> tween_;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_ANIMATION_H__
