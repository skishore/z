#include <map>
#include <string>
#include <vector>

#include "base/debug.h"
#include "ui/Animation.h"

using std::map;
using std::move;
using std::string;
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

static const int kAttackFrames = 4;

class CheckpointComponent : public AnimationComponent {
 public:
  bool Update() override {
    return false;
  };

  void Draw(const engine::View& view, Graphics* graphics) const override {
    ASSERT(false, "CheckpointComponent's Draw should not be called!");
  };
};

class AttackComponent : public AnimationComponent {
 public:
  AttackComponent(const Point& square) : square_(square) {}

  bool Update() override {
    frame_ += 1;
    return frame_ < kAttackFrames;
  };

  void Draw(const engine::View& view, Graphics* graphics) const override {
    graphics->Clear();
    graphics->Draw(view);
    graphics->ShadeSquare(view, square_, 0x00ff0000, 0.5);
    graphics->Flip();
  };

 private:
  int frame_ = 0;
  Point square_;
};

}

Animation::Animation(const engine::GameState& game_state)
    : game_state_(game_state) {
  last_.reset(Snapshot());
}

Animation::~Animation() {
  for (auto& step : steps_) {
    delete step.component;
    delete step.view;
  }
}

void Animation::AfterAttack(const engine::Sprite& sprite,
                            const engine::Sprite& target) {
  PushStep(AnimationStep{new AttackComponent(target.square), Snapshot()});
}

void Animation::Checkpoint() {
  PushStep(AnimationStep{new CheckpointComponent, Snapshot()});
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

engine::View* Animation::Snapshot() const {
  return new engine::View(kScreenRadius, game_state_);
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
  ASSERT(tween_ == nullptr, "PopStep called while a tween was running!");
  ASSERT(!steps_.empty(), "PopStep called when steps_ was empty!");
  const AnimationStep& step = steps_[0];
  delete step.component;
  last_.reset(step.view);
  steps_.pop_front();
  if (!steps_.empty()) {
    tween_.reset(new Tween(*last_, *steps_[0].view));
  }
}

} // namespace ui 
} // namespace babel
