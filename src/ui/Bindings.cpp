#include <map>
#include <memory>

#include "base/debug.h"
#include "base/point.h"
#include "engine/Action.h"
#include "engine/View.h"
#include "ui/Bindings.h"

namespace babel {
namespace ui {

namespace {

static const int kFrameRate = 60;
static const int kScreenRadius = 10;

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
    : verbose_(verbose), graphics_(kScreenRadius, interface_) {}

int Bindings::Start() {
  Reset();
  GameLoop(kFrameRate, this);
  return 0;
}

bool Bindings::Update(double frame_rate) {
  static double last_frame_rate = 0;
  if (frame_rate != last_frame_rate && verbose_) {
    last_frame_rate = frame_rate;
    DEBUG("FPS: " << frame_rate);
  }

  char ch;
  bool has_input = input_.GetChar(&ch);
  if (has_input && ch == 0x1b) {
    return false;
  }

  if (animation_->Update()) {
    animation_->Draw(&graphics_);
    return true;
  } else if (!has_input) {
    return true;
  }

  engine::Action* input = nullptr;
  bool redraw = false;
  if (interface_.Consume(ch, &input, &redraw)) {
    if (input != nullptr) {
      if (engine_->Update(input)) {
        redraw = true;
        interface_.Clear();
      }
    }
    if (redraw) {
      Redraw();
    }
  } else if (ch == 'r') {
    Reset();
  } else if (kShift.find(ch) != kShift.end()) {
    if (engine_->Update(new engine::MoveAction(kShift.at(ch)))) {
      interface_.Clear();
      Redraw();
    }
  }

  return true;
}

void Bindings::Reset() {
  engine_.reset(new engine::Engine());
  animation_.reset(new Animation(kScreenRadius, engine_->GetGameState()));
  engine_->AddEventHandler(animation_.get());
  Redraw();
}

void Bindings::Redraw() {
  animation_->Checkpoint();
  animation_->Update();
  animation_->Draw(&graphics_);
}

}  // namespace ui
}  // namespace babel
