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

interface::DialogResult Interface::Consume(char ch) {
  interface::Dialog* dialog = engine_->GetDialog();
  if (dialog != nullptr) {
    return dialog->Consume(ch);
  }
  return interface::DialogResult();
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
