#include <assert.h>

#include "Engine.h"

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
}  // namespace

Engine::Engine(int frame_rate, const Point& screen_size)
    : screen_size_(screen_size) {
  assert(map_.LoadMap("world.dat"));

  graphics_.reset(new ScrollingGraphics(screen_size_, map_));
  graphics_->RedrawBackground();
  graphics_->EraseForeground();
  graphics_->Flip();

  GameLoop(frame_rate, this);
}

bool Engine::Update() {
  graphics_->RedrawBackground();
  graphics_->EraseForeground();
  graphics_->Flip();

  return CheckForExitEvents();
}

}  // namespace skishore
