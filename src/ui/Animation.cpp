#include <map>
#include <vector>

#include "base/debug.h"
#include "ui/Animation.h"

using std::map;
using std::move;
using std::unique_ptr;
using std::vector;

namespace babel {
namespace ui {

class AnimationComponent {
 public:
  virtual ~AnimationComponent() {};

  // Returns false if this AnimationComponent is complete.
  virtual bool Update() = 0;
  virtual void Draw(const engine::View& view, Graphics* graphics) const = 0;
};

namespace {

class CheckpointComponent : public AnimationComponent {
 public:
  bool Update() override {
    return false;
  };

  void Draw(const engine::View& view, Graphics* graphics) const override {
    ASSERT(false, "CheckpointComponent's Draw should not be called!");
  };
};

}

Animation::Animation(const engine::GameState& game_state)
    : game_state_(game_state) {
  last_.reset(new engine::View(kScreenRadius, game_state_));
}

void Animation::Checkpoint() {
  engine::View* view = new engine::View(kScreenRadius, game_state_);
  PushStep(AnimationStep{new CheckpointComponent, view});
  Update();
}

void Animation::Draw(Graphics* graphics) const {
  if (tween_ != nullptr) {
    tween_->Draw(graphics);
  } else if (!steps_.empty()) {
    const AnimationStep& step = steps_[0];
    step.component->Draw(*step.view, graphics);
  } else {
    ASSERT(false, "Draw called when no animation was running!");
  }
}

bool Animation::Update() {
  while (!steps_.empty()) {
    if (tween_ != nullptr && tween_->Update()) {
      return true;
    }
    tween_.reset(nullptr);
    if (steps_[0].component->Update()) {
      return true;
    }
    PopStep();
  }
  return !steps_.empty();
}

void Animation::PushStep(const AnimationStep& step) {
  ASSERT(step.component != nullptr, "component was NULL!");
  ASSERT(step.view != nullptr, "view was NULL!");
  if (tween_ == nullptr) {
    tween_.reset(new Tween(*last_, *step.view));
  }
  steps_.push_back(step);
}

void Animation::PopStep() {
  ASSERT(!steps_.empty(), "PopStep called when steps_ was empty!");
  const AnimationStep& step = steps_[0];
  delete step.component;
  last_.reset(step.view);
  steps_.pop_front();
}

} // namespace ui 
} // namespace babel
