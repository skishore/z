#include "Bindings.h"

#include <memory>

#include "constants.h"
#include "debug.h"
#include "Engine.h"
#include "Point.h"
#include "View.h"

namespace babel {

Bindings::Bindings(Engine* engine)
    : engine_(engine), graphics_(Point(NCOLS, NROWS)) {
  ASSERT(engine_, "engine_ == nullptr");
}

int Bindings::Start() {
  Redraw();

  char ch;
  while (input_.GetChar(&ch)) {
    if (ch == 0x03 || ch == 0x1B /* ctrl-C and escape */) {
      break;
    } else if (engine_->HandleCommand(ch)) {
      Redraw();
    }
  }
  return 0;
}

void Bindings::Redraw() {
  graphics_.Clear();
  View view(kScreenRadius, engine_->GetGameState());
  graphics_.Draw(view);
  graphics_.Flip();
}

}  // namespace babel
