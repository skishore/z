#include "debug.h"
#include "GameState.h"
#include "SpriteState.h"

namespace skishore {

void SpriteState::Register(Sprite* sprite) {
  ASSERT(sprite != nullptr, "Registering NULL sprite!");
  ASSERT(sprite_ == nullptr, "Re-registering sprite!");
  sprite_ = sprite;
}

SpriteState* WalkingState::MaybeTransition(const GameState& game_state) {
  return nullptr;
}

Position* WalkingState::Move(const GameState& game_state) {
  if (game_state.input_.IsKeyPressed(SDLK_DOWN)) {
    sprite_->frame_.x = Direction::DOWN;
  }
  return nullptr;
}

} // namespace skishore
