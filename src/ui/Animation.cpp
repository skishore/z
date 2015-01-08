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

Animation::Animation(const engine::GameState& game_state)
    : game_state_(game_state) {}

void Animation::Checkpoint() {
  ASSERT(next_ == nullptr, "Checkpoint called while animating!");
  next_.reset(new engine::View(kScreenRadius, game_state_));
  if (last_ != nullptr) {
    tween_.reset(new Tween(*last_, *next_));
  }
  Update();
}

void Animation::Draw() {
  if (tween_ != nullptr) {
    tween_->Draw(&graphics_);
  } else if (next_ != nullptr) {
    last_.reset(next_.release());
    graphics_.Clear();
    graphics_.Draw(*last_);
    graphics_.Flip();
  }
}

bool Animation::Update() {
  if (next_ != nullptr) {
    if (tween_ != nullptr && tween_->Update()) {
      tween_.reset(nullptr);
    }
  }
  return next_ == nullptr;
}

} // namespace ui 
} // namespace babel
