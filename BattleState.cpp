#include "constants.h"
#include "BattleData.h"
#include "BattleState.h"

namespace skishore {
namespace battle {

namespace {

const static int kBattleSpeed = 1.2*kPlayerSpeed;
const static int kTolerance = kPlayerSpeed;

Direction GetDirection(const Point& move) {
  return (move.x < 0 ? Direction::LEFT : Direction::RIGHT);
}

}  // namespace

SpriteState* FaceTargetState::MaybeTransition(
    const GameState& game_state) const {
  return nullptr;
}

SpriteState* FaceTargetState::Update(const GameState& game_state) {
  Point move = target_ - sprite_->GetPosition();
  sprite_->dir_ = GetDirection(move);
  sprite_->frame_.x = sprite_->dir_;
  return new WaitingState;
}

SpriteState* SpeakState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* SpeakState::Update(const GameState& game_state) {
  sprite_->battle_->dir = dir_;
  sprite_->battle_->text = text_;
  index_ += 1;
  if (index_ >= 60) {
    sprite_->battle_->text = "";
    return new WaitingState;
  }
  return nullptr;
}

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
  Point move = target_ - sprite_->GetPosition();
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
