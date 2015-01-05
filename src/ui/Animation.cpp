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

void Animation::SetNextView(engine::View* view) {
  ASSERT(next_ == nullptr, "SetNextView called with an animation running!");
  next_.reset(view);
  if (last_ != nullptr) {
    tween_.reset(new Tween(*last_, *next_));
  }
}

bool Animation::Update() {
  if (next_ != nullptr) {
    if (tween_ != nullptr && !tween_->Update()) {
      tween_->Draw(&graphics_);
    } else {
      Commit();
    }
  }
  return next_ == nullptr;
}

void Animation::Commit() {
  last_.reset(next_.release());
  graphics_.Clear();
  graphics_.Draw(*last_);
  graphics_.Flip();
}

} // namespace ui 
} // namespace babel
