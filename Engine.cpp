#include <string>
#include <SDL2/SDL.h>

#include "debug.h"
#include "util.h"
#include "Engine.h"
#include "Sprite.h"

using std::string;

namespace skishore {

namespace {
static const int kEventsPerFrame = 16;
}  // namespace

Engine::Engine(int frame_rate, const Point& screen_size)
    : screen_size_(screen_size) {
  map_.LoadMap("world.dat");

  graphics_.reset(new ScrollingGraphics(screen_size_, map_));
  graphics_->RedrawBackground();
  game_state_.reset(new GameState(input_, map_, graphics_->GetImageCache()));

  GameLoop(frame_rate, this);
}

bool Engine::Update(double frame_rate) {
  input_.Poll(kEventsPerFrame);
  game_state_->Update();

  graphics_->CenterCamera(game_state_->GetPlayer());
  graphics_->EraseForeground();
  for (const Sprite* sprite : game_state_->GetSprites()) {
    graphics_->DrawSprite(*sprite);
  }
  graphics_->DrawStatusMessage("FPS: " + DoubleToString(frame_rate, 2));
  graphics_->Flip();

  return input_.IsExitSignaled();
}

}  // namespace skishore
