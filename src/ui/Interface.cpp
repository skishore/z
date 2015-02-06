#include "base/debug.h"
#include "ui/Interface.h"

namespace babel {
namespace ui {

void Interface::Register(engine::Engine* engine) {
  ASSERT(engine != nullptr, "engine_ == nullptr!");
  engine_ = engine;
  Clear();
}

void Interface::Clear() {
  active_ = false;
}

bool Interface::Consume(char ch, engine::Action** action, bool* redraw) {
  interface::Dialog* dialog = engine_->GetDialog();
  if (dialog != nullptr) {
    return dialog->Consume(ch, action, redraw);
  }
  return false;
}

bool Interface::Active() const {
  return (active_ || engine_->GetDialog() != nullptr);
}

void Interface::Draw(render::DialogRenderer* renderer) const {
  ASSERT(Active(), "Draw called when the dialog was inactive!");
  interface::Dialog* dialog = engine_->GetDialog();
  if (dialog != nullptr) {
    dialog->Draw(renderer);
  }
}

}  // namespace ui
}  // namespace babel
