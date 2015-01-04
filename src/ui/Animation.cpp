#include "base/debug.h"
#include "ui/Animation.h"

namespace babel {
namespace ui {

namespace {
static const int kGridSize = 16;
static const int kTweenFrames = 4;
}  // namespace

void Animation::HandleAttack(
    const engine::Sprite& source, const engine::Sprite& target) {}

void Animation::HandleMove(
    const engine::Sprite& sprite, const Point& square) {}

void Animation::SetNextView(engine::View* view) {
  ASSERT(next_ == nullptr, "SetNextView called with an animation running!");
  next_.reset(view);
}

bool Animation::Update() {
  if (next_ != nullptr) {
    frame += 1;
    if (frame == kTweenFrames || last_ == nullptr) {
      Commit();
    } else {
      DrawTweenFrame();
    }
  }
  return next_ == nullptr;
}

void Animation::Commit() {
  last_.reset(next_.release());
  graphics_.Clear();
  graphics_.Draw(*last_);
  graphics_.Flip();
  frame = 0;
}

void Animation::DrawTweenFrame() {
}

} // namespace ui 
} // namespace babel
