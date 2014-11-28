#include "constants.h"
#include "BattleState.h"

namespace skishore {
namespace battle {

namespace {

const static int kBattleSpeed = 1.2*kPlayerSpeed;
const static int kTolerance = kPlayerSpeed;

Direction GetDirection(const Point& move) {
  if (abs(move.x) > abs(move.y)) {
    return (move.x < 0 ? Direction::LEFT : Direction::RIGHT);
  }
  return (move.y < 0 ? Direction::UP : Direction::DOWN);
}

}  // namespace

SpriteState* WaitingState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* WaitingState::Update(const GameState& game_state) {
  return nullptr;
}

SpriteState* WalkToTargetState::MaybeTransition(
    const GameState& game_state) const {
  return nullptr;
}

SpriteState* WalkToTargetState::Update(const GameState& game_state) {
  sprite_->frame_.x = sprite_->dir_;
  Point move = kGridTicks*target_ - sprite_->GetPosition();
  if (move.length() > kTolerance) {
    sprite_->dir_ = GetDirection(move);
    move.set_length(kBattleSpeed);
    MoveSprite(game_state, sprite_, &move, &anim_num_);
    return nullptr;
  }
  return new WaitingState;
}

} // namespace battle
} // namespace skishore
