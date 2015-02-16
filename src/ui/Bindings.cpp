#include "base/debug.h"
#include "ui/Bindings.h"

namespace babel {
namespace ui {

namespace {
static const int kFrameRate = 60;
static const int kScreenRadius = 10;
}  // namespace

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

  interface::DialogResult result = interface_.Consume(ch);
  if (result.reset) {
    Reset();
  } else if (result.action != nullptr || result.update) {
    if (engine_->Update(result.action)) {
      result.redraw = true;
      interface_.Clear();
    }
  }
  if (result.redraw) {
    Redraw();
  } else if (result.redraw_dialog) {
    // Partial draws don't work in the WebGL implementation of SDL2.
    #ifdef EMSCRIPTEN
    Redraw();
    #else
    graphics_.DrawDialog();
    #endif
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
