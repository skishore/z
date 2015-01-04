#include "base/debug.h"
#include "ui/Animation.h"

namespace babel {
namespace ui {

void Animation::HandleAttack(const Point& source, const Point& target) {}
void Animation::HandleMove(const Point& source, const Point& target) {}

void Animation::SetNextView(engine::View* view) {
  ASSERT(next_ == nullptr, "SetNextView called with an animation running!");
  next_.reset(view);
}

bool Animation::Update() {
  if (next_ != nullptr) {
    last_.reset(next_.release());
    graphics_.Clear();
    graphics_.Draw(*last_);
    graphics_.Flip();
  }
  return next_ == nullptr;
}

} // namespace ui 
} // namespace babel
