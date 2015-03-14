#include "ui/Bindings.h"

#include <memory>

#include "base/point.h"
#include "engine/View.h"

namespace babel {
namespace ui {
namespace {

// The screen dimensions, in grid cells.
static const Point radius(31, 11);

}  // namespace

Bindings::Bindings() {
  Reset();

  while (true) {
    InputResult result = handler_.GetInput();
    if (result.done) {
      break;
    } else if (result.reset) {
      Reset();
      continue;
    } else if (result.action == nullptr) {
      continue;
    }

    engine_->AddInput(result.action);
    bool changed = false;
    while (engine_->Update()) {
      changed = true;
    }

    if (!changed) {
      continue;
    }
    std::unique_ptr<engine::View> view(engine_->GetView(radius));
    graphics_.Redraw(*view);
  }
}

void Bindings::Reset() {
  engine_.reset(new engine::Engine);
  std::unique_ptr<engine::View> view(engine_->GetView(radius));
  graphics_.Redraw(*view);
}

}  // namespace ui
}  // namespace babel
