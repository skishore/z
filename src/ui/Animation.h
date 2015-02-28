#ifndef __BABEL_UI_ANIMATION_H__
#define __BABEL_UI_ANIMATION_H__

#include <deque>
#include <memory>

#include "engine/Engine.h"
#include "engine/EventHandler.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "ui/Transform.h"
#include "ui/Tween.h"

namespace babel {
namespace ui {

class AnimationComponent;

struct AnimationResult {
  const engine::View* view = nullptr;
  const Transform* transform = nullptr;
};

struct AnimationStep {
  AnimationComponent* component;
  engine::View* view;
};

class Animation : public engine::EventHandler {
 public:
  Animation(int radius, engine::Engine* engine);
  ~Animation();

  // EventHandler callbacks, used to add to the animation queue.
  void BeforeAttack(const engine::Sprite& sprite,
                    const engine::Sprite& target) override;

  AnimationResult Update();

 private:
  // The caller takes ownership of the new View.
  inline engine::View* Snapshot() const;
  void PushStep(const AnimationStep& step);
  void PopStep();

  const int radius_;
  engine::Engine* engine_;

  std::unique_ptr<engine::View> last_;
  std::unique_ptr<Tween> tween_;
  std::deque<AnimationStep> steps_;
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_UI_ANIMATION_H__
