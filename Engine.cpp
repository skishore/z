#include <string>

#include "debug.h"
#include "Engine.h"

using std::string;
using std::to_string;

namespace skishore {

namespace {

static const int kEventsPerFrame = 16;

bool CheckForExitEvents() {
  SDL_Event event;
  for (int i = 0; (i < kEventsPerFrame) && SDL_PollEvent(&event); i++) {
    if (event.type == SDL_QUIT ||
        (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
      return true;
    }
  }
  return false;
}

string GetStatusMessage(double frame_rate) {
  return "FPS: " + (frame_rate > 0 ? std::to_string(frame_rate) : "-");
}

}  // namespace

Engine::Engine(int frame_rate, const Point& screen_size)
    : screen_size_(screen_size) {
  ASSERT(map_.LoadMap("world.dat"), "Failed to load map.");

  graphics_.reset(new ScrollingGraphics(screen_size_, map_));
  graphics_->RedrawBackground();
  graphics_->EraseForeground();
  graphics_->Flip();

  GameLoop(frame_rate, this);
}

bool Engine::Update(double frame_rate) {
  graphics_->RedrawBackground();
  graphics_->EraseForeground();
  graphics_->DrawStatusMessage(GetStatusMessage(frame_rate));
  graphics_->Flip();

  return CheckForExitEvents();
}

}  // namespace skishore
