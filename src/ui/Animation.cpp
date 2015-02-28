#include "ui/Animation.h"

#include <map>
#include <string>
#include <vector>

#include "base/debug.h"

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
  virtual void Draw(const Transform** transform) const = 0;
};

namespace {

class CheckpointComponent : public AnimationComponent {
 public:
  bool Update() override {
    return false;
  };

  void Draw(const Transform** transform) const override {
    ASSERT(false, "CheckpointComponent's Draw should not be called!");
  };
};

class TransformComponent : public AnimationComponent {
 public:
  bool Update() override {
    frames_left_ -= 1;
    return frames_left_ >= 0;
  }

  void Draw(const Transform** transform) const override {
    *transform = &transform_;
  };

 protected:
  int frames_left_;
  Transform transform_;
};

class AttackComponent : public TransformComponent {
 public:
  AttackComponent(const Point& square) {
    frames_left_ = 4;
    transform_.shaded_squares[square] = Transform::Shade{0x00ff0000, 0.5};
  }
};

}  // namespace

Animation::Animation(int radius, engine::Engine* engine)
    : radius_(radius), engine_(engine) {
  last_.reset(Snapshot());
}

Animation::~Animation() {
  for (auto& step : steps_) {
    delete step.component;
    delete step.view;
  }
}

void Animation::BeforeAttack(const engine::Sprite& sprite,
                             const engine::Sprite& target) {
  PushStep(AnimationStep{new AttackComponent(target.square), Snapshot()});
}

AnimationResult Animation::Update() {
  AnimationResult result;
  while (!steps_.empty()) {
    if (tween_ != nullptr && tween_->Update()) {
      tween_->Draw(&result.view, &result.transform);
      return result;
    }
    tween_.reset(nullptr);
    if (steps_[0].component->Update()) {
      const AnimationStep& step = steps_[0];
      result.view = step.view;
      step.component->Draw(&result.transform);
      return result;
    }
    PopStep();
    if (steps_.empty() && engine_->Update()) {
      PushStep(AnimationStep{new CheckpointComponent, Snapshot()});
    }
  }
  return result;
}

engine::View* Animation::Snapshot() const {
  return engine_->GetView(radius_);
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
  if (steps_.empty()) {
    last_->log.clear();
  } else {
    tween_.reset(new Tween(*last_, *steps_[0].view));
  }
}

} // namespace ui 
} // namespace babel
