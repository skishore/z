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

Bindings::Bindings(bool verbose)
    : verbose_(verbose), animation_(engine_.GetGameState()) {
  engine_.AddEventHandler(&animation_);
}

int Bindings::Start() {
  Redraw();
  GameLoop(kFrameRate, this);
  return 0;
}

bool Bindings::Update(double frame_rate) {
  static double last_frame_rate = 0;
  if (frame_rate != last_frame_rate && verbose_) {
    last_frame_rate = frame_rate;
    DEBUG("FPS: " << frame_rate);
  }

  if (animation_.Update()) {
    animation_.Draw(&graphics_);
    return true;
  }
  std::unique_ptr<engine::Action> input;
  char ch;
  if (input_.GetChar(&ch)) {
    if (ch == 0x03 || ch == 0x1B /* ctrl-C, escape */) {
      return false;
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
  return true;
}

void Bindings::Redraw() {
  animation_.Checkpoint();
  animation_.Draw(&graphics_);
}

}  // namespace ui
}  // namespace babel
