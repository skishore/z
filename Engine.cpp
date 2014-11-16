#include <string>
#include <SDL2/SDL.h>

#include "debug.h"
#include "util.h"
#include "Engine.h"

using std::string;

namespace skishore {

namespace {
static const int kEventsPerFrame = 16;
static const Point kStartingSquare(8, 8);
}  // namespace

Engine::Engine(int frame_rate, const Point& screen_size)
    : screen_size_(screen_size) {
  ASSERT(map_.LoadMap("world.dat"), "Failed to load map.");

  graphics_.reset(new ScrollingGraphics(screen_size_, map_));
  graphics_->RedrawBackground();

  player_.reset(new Sprite(
      kStartingSquare, *graphics_->LoadImage("player.bmp")));
  graphics_->CenterCamera(*player_);

  GameLoop(frame_rate, this);
}

bool Engine::Update(double frame_rate) {
  input_.Poll(kEventsPerFrame);
  if (input_.IsKeyPressed(SDLK_UP)) {
    player_->position_.y -= 1;
  }
  if (input_.IsKeyPressed(SDLK_DOWN)) {
    player_->position_.y += 1;
  }
  if (input_.IsKeyPressed(SDLK_RIGHT)) {
    player_->position_.x += 1;
  }
  if (input_.IsKeyPressed(SDLK_LEFT)) {
    player_->position_.x -= 1;
  }

  graphics_->CenterCamera(*player_);
  graphics_->EraseForeground();
  graphics_->DrawSprite(*player_);
  graphics_->DrawStatusMessage("FPS: " + DoubleToString(frame_rate, 2));
  graphics_->Flip();

  return input_.IsExitSignaled();
}

}  // namespace skishore
