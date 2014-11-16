#include <string>
#include <SDL2/SDL.h>

#include "debug.h"
#include "util.h"
#include "Engine.h"

using std::string;

namespace skishore {

namespace {
static const int kEventsPerFrame = 16;
}  // namespace

Engine::Engine(int frame_rate, const Point& screen_size)
    : screen_size_(screen_size) {
  ASSERT(map_.LoadMap("world.dat"), "Failed to load map.");
  graphics_.reset(new ScrollingGraphics(screen_size_, map_));
  GameLoop(frame_rate, this);
}

bool Engine::Update(double frame_rate) {
  static Point point;

  input_.Poll(kEventsPerFrame);
  Point last_point = point;
  if (input_.IsKeyPressed(SDLK_UP)) {
    point.y -= 1;
  }
  if (input_.IsKeyPressed(SDLK_DOWN)) {
    point.y += 1;
  }
  if (input_.IsKeyPressed(SDLK_RIGHT)) {
    point.x += 1;
  }
  if (input_.IsKeyPressed(SDLK_LEFT)) {
    point.x -= 1;
  }
  if (point != last_point) {
    graphics_->CenterCamera(point);
  }

  graphics_->RedrawBackground();
  graphics_->EraseForeground();
  graphics_->DrawStatusMessage("FPS: " + DoubleToString(frame_rate, 2));
  graphics_->Flip();

  return input_.IsExitSignaled();
}

}  // namespace skishore
