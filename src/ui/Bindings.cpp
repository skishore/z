#include "base/debug.h"
#include "base/timing.h"
#include "ui/Bindings.h"

namespace babel {
namespace ui {

namespace {
static const int kFrameRate = 60;
static const int kScreenRadius = 10;
}  // namespace

Bindings::Bindings(bool verbose) : graphics_(kScreenRadius, interface_) {
  SetTimerVerbosity(verbose);
}

int Bindings::Start() {
  Reset();
  GameLoop(kFrameRate, this);
  return 0;
}

bool Bindings::Update(double frame_rate) {
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

  interface::DialogResult result = interface_.Consume(ch);
  if (result.reset) {
    Reset();
  } else if (result.action != nullptr || result.update) {
    StartTimer("Engine::Update");
    if (engine_->Update(result.action)) {
      result.redraw = true;
      interface_.Clear();
    }
    EndTimer();
  }
  if (result.redraw) {
    StartTimer("Animation::Draw");
    Redraw();
    EndTimer();
  } else if (result.redraw_dialog) {
    StartTimer("Graphics::DrawDialog");
    // Partial draws don't work in the WebGL implementation of SDL2.
    #ifdef EMSCRIPTEN
    Redraw();
    #else
    graphics_.DrawDialog();
    #endif
    EndTimer();
  }
  return true;
}

void Bindings::Reset() {
  engine_.reset(new engine::Engine());
  animation_.reset(new Animation(kScreenRadius, engine_->GetGameState()));
  engine_->AddEventHandler(animation_.get());
  interface_.Register(engine_.get());
  Redraw();
}

void Bindings::Redraw() {
  animation_->Checkpoint();
  animation_->Update();
  animation_->Draw(&graphics_);
}

}  // namespace ui
}  // namespace babel
