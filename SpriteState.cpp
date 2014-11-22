#include <algorithm>

#include "constants.h"
#include "debug.h"
#include "GameState.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

namespace {
const int kWalkingAnimationFrames = 12;
}  // namespace

void SpriteState::Register(Sprite* sprite) {
  ASSERT(sprite != nullptr, "Registering NULL sprite!");
  ASSERT(sprite_ == nullptr, "Re-registering sprite!");
  sprite_ = sprite;
}

SpriteState* WalkingState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* WalkingState::Update(const GameState& game_state) {
  Direction last_dir = sprite_->direction_;
  Point move;

  for (int dir = 0; dir < 4; dir++) {
    if (game_state.input_.IsKeyPressed(kDirectionKey[dir])) {
      last_dir = (Direction)dir;
      move += kShift[dir];
    }
  }
  if (last_dir != sprite_->direction_ &&
      !game_state.input_.IsKeyPressed(kDirectionKey[sprite_->direction_])) {
    sprite_->direction_ = last_dir;
  }
  sprite_->frame_.x = sprite_->direction_;

  move.set_length(kPlayerSpeed);
  sprite_->AvoidOthers(game_state.sprites_, &move);
  if (sprite_->Move(game_state.map_, &move)) {
    if (anim_num_ % kWalkingAnimationFrames >= kWalkingAnimationFrames/2) {
      sprite_->frame_.x += 4;
    }
    anim_num_ = (anim_num_ + 1) % kWalkingAnimationFrames;
  }
  return nullptr;
}

} // namespace skishore
