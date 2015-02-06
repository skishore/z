#ifndef __BABEL_UI_ANIMATION_H__
#define __BABEL_UI_ANIMATION_H__

#include <deque>
#include <memory>

#include "engine/EventHandler.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "render/Graphics.h"
#include "ui/Tween.h"

namespace babel {
namespace ui {

class AnimationComponent;

struct AnimationStep {
  AnimationComponent* component;
  engine::View* view;
};

class Animation : public engine::EventHandler {
 public:
  Animation(int radius, const engine::GameState& game_state);
  ~Animation();

  // EventHandler callbacks, used to add to the animation queue.
  void BeforeAttack(const engine::Sprite& sprite,
                    const engine::Sprite& target) override;
  void BeforeSpeech(const engine::Sprite& sprite) override;

  void Checkpoint();
  void Draw(render::Graphics* graphics) const;

  // Returns false if the current animation is complete.
  bool Update();

 private:
  // The caller takes ownership of the new View.
  inline engine::View* Snapshot() const;
  void PushStep(const AnimationStep& step);
  void PopStep();

  const int radius_;
  const engine::GameState& game_state_;

  std::unique_ptr<engine::View> last_;
  std::unique_ptr<Tween> tween_;
  std::deque<AnimationStep> steps_;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_UI_ANIMATION_H__
