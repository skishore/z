#include "base/debug.h"
#include "ui/Interface.h"

namespace babel {
namespace ui {

void Interface::Clear() {
  active_ = false;
}

bool Interface::Consume(char ch, engine::Action** action, bool* redraw) {
  return false;
}

bool Interface::Active() const {
  return active_;
}

void Interface::Draw(render::DialogRenderer* renderer) const {
  ASSERT(active_, "Draw called when the dialog was inactive!");
}

}  // namespace ui
}  // namespace babel
