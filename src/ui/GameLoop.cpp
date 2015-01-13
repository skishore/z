#include <unistd.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

#include "base/debug.h"
#include "base/timing.h"
#include "ui/GameLoop.h"

namespace babel {
namespace ui {
namespace {

static const tick kTicksPerSecond = 1000000;
static const tick kBreak = 4000;
static const int kUpdatesPerFrame = 1;

// We store the current frame rate and updatable in static global variables
// because the emscripten main loop must be a global void(void) function.
static int frame_rate_ = 0;
static GameLoop::Updatable* updatable_ = nullptr;
#ifdef EMSCRIPTEN
static const bool emscripten_ = true;
#else
static const bool emscripten_ = false;
#endif

// This function executes one update when emscripten is false but loops when
// emscripten is true.
void UpdateLoop() {
  static tick cur_time = 0, last_time = 0, last_second = 0;
  static const tick kTicksPerFrame = kTicksPerSecond/frame_rate_;
  static int frames = 0;
  static double frame_rate = 0;

  bool done = emscripten_;

  do {
    cur_time = GetCurrentTick();
    if (!emscripten_ && cur_time < last_time + kTicksPerFrame) {
      continue;
    }

    int updates = (cur_time - last_time)/kTicksPerFrame;
    updates = std::max(std::min(updates, kUpdatesPerFrame), 1);
    for (int i = 0; i < updates; i++) {
      done |= !updatable_->Update(frame_rate);
    }

    if (cur_time > last_second + kTicksPerSecond) {
      if (frames > 0) {
        frame_rate = 1.0*frames*kTicksPerSecond/(cur_time - last_second);
      }
      last_second = cur_time;
      frames = 0;
    }
    last_time = cur_time;
    frames++;

    if (!emscripten_) {
      tick delay = cur_time + kTicksPerFrame - GetCurrentTick() - kBreak;
      if (delay > 0) {
        usleep(delay);
      }
    }
  } while (!done);
}

}  // namespace

GameLoop::GameLoop(int frame_rate, GameLoop::Updatable* updatable) {
  ASSERT(updatable != nullptr, "No updatable for game loop!");
  ASSERT(updatable_ == nullptr, "Started a second game loop!");
  updatable_ = updatable;
  frame_rate_ = frame_rate;

  #ifdef EMSCRIPTEN
  emscripten_set_main_loop(UpdateLoop, 0, true);
  #else
  UpdateLoop();
  #endif
}

}  // namespace ui
}  // namespace babel
