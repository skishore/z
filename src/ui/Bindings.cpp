#include <map>
#include <memory>

#include "base/constants.h"
#include "base/debug.h"
#include "base/point.h"
#include "engine/Action.h"
#include "engine/View.h"
#include "ui/Bindings.h"

namespace babel {
namespace ui {

namespace {

static const int kFrameRate = 60;

static const std::map<char,Point> kShift = {
  {'h', Point(-1, 0)},
  {'j', Point(0, 1)},
  {'k', Point(0, -1)},
  {'l', Point(1, 0)},
  {'y', Point(-1, -1)},
  {'u', Point(1, -1)},
  {'b', Point(-1, 1)},
  {'n', Point(1, 1)},
  {'.', Point(0, 0)}
};

}

Bindings::Bindings() : engine_(&animation_) {}

int Bindings::Start() {
  Redraw();
  GameLoop(kFrameRate, this);
  return 0;
}

bool Bindings::Update(double frame_rate) {
  if (!animation_.Update()) {
    return false;
  }
  std::unique_ptr<engine::Action> input;
  char ch;
  if (input_.GetChar(&ch)) {
    if (ch == 0x03 || ch == 0x1B /* ctrl-C, escape */) {
      return true;
    } else if (kShift.find(ch) != kShift.end()) {
      input.reset(new engine::MoveAction(kShift.at(ch)));
    }
  }
  bool used_input = false;
  if (engine_.Update(input.get(), &used_input)) {
    Redraw();
  }
  if (used_input) {
    input.release();
  }
  return false;
}

void Bindings::Redraw() {
  animation_.SetNextView(new engine::View(
      kScreenRadius, engine_.GetGameState()));
  animation_.Update();
}

}  // namespace ui
}  // namespace babel
