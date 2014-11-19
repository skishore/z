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

Point* WalkingState::GetMove(const GameState& game_state) {
  Point move;
  bool moved = false;
  Direction last_dir = sprite_->direction_;

  for (int dir = 0; dir < 4; dir++) {
    if (game_state.input_.IsKeyPressed(kDirectionKey[dir])) {
      last_dir = (Direction)dir;
      move += kShift[dir];
      moved = true;
    }
  }
  if (last_dir != sprite_->direction_ &&
      !game_state.input_.IsKeyPressed(kDirectionKey[sprite_->direction_])) {
    sprite_->direction_ = last_dir;
  }
  sprite_->frame_.x = sprite_->direction_;

  if (moved) {
    move.set_length(kPlayerSpeed);
    return new Point(move);
  }
  return nullptr;
}

} // namespace skishore
