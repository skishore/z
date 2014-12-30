#include <memory>

#include "base/constants.h"
#include "base/debug.h"
#include "base/Point.h"
#include "engine/View.h"
#include "ui/Bindings.h"

namespace babel {
namespace ui {

namespace {
static const int kFrameRate = 60;
}

Bindings::Bindings(engine::Engine* engine)
    : engine_(engine), graphics_(Point(NCOLS, NROWS)) {
  ASSERT(engine_, "engine_ == nullptr");
}

int Bindings::Start() {
  Redraw();
  GameLoop(kFrameRate, this);
  return 0;
}

bool Bindings::Update(double frame_rate) {
  char ch;
  bool has_input = input_.GetChar(&ch);
  if (has_input && (ch == 0x03 || ch == 0x1B /* ctrl-C, escape */)) {
    return true;
  } else if (engine_->Update(has_input, ch)) {
    Redraw();
  }
  return false;
}

void Bindings::Redraw() {
  graphics_.Clear();
  engine::View view(kScreenRadius, engine_->GetGameState());
  graphics_.Draw(view);
  graphics_.Flip();
}

}  // namespace ui
}  // namespace babel
