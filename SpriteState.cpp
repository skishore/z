#include <algorithm>

#include "constants.h"
#include "debug.h"
#include "GameState.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

void SpriteState::Register(Sprite* sprite) {
  ASSERT(sprite != nullptr, "Registering NULL sprite!");
  ASSERT(sprite_ == nullptr, "Re-registering sprite!");
  sprite_ = sprite;
}

SpriteState* WalkingState::MaybeTransition(const GameState& game_state) {
  return nullptr;
}

Position* WalkingState::GetMove(const GameState& game_state) {
  if (game_state.input_.IsKeyPressed(SDLK_DOWN)) {
    sprite_->frame_.x = Direction::DOWN;
  }
  return nullptr;
}

} // namespace skishore
