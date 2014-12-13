#include "constants.h"
#include "BattleData.h"
#include "BattleState.h"

using std::string;

namespace skishore {
namespace battle {

namespace {

const static int kBattleSpeed = 1.2*kPlayerSpeed;
const static int kTolerance = kPlayerSpeed;

// Constants controlling speaking speed.
const static int kFramesPerGlyph = kFrameRate/30;
const static int kFinalDelay = kFrameRate;

Direction GetDirection(const Point& move, bool force_horizontal) {
  if (abs(move.x) > abs(move.y) || force_horizontal) {
    return (move.x < 0 ? Direction::LEFT : Direction::RIGHT);
  }
  return (move.y < 0 ? Direction::UP : Direction::DOWN);
}

void AdvanceTextIndex(const string& text, int* index) {
  // TODO(skishore): Do some more complex logic here for Devanagari text.
  *index += 1;
}

}  // namespace

SpriteState* FaceTargetState::MaybeTransition(
    const GameState& game_state) const {
  return nullptr;
}

SpriteState* FaceTargetState::Update(const GameState& game_state) {
  Point move = target_ - sprite_->GetPosition();
  sprite_->dir_ = GetDirection(move, true /* force_horizontal */);
  sprite_->frame_.x = sprite_->dir_;
  return new WaitingState;
}

SpriteState* SpeakState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* SpeakState::Update(const GameState& game_state) {
  if (frame_ <= 0) {
    if (index_ >= text_.size()) {
      sprite_->battle_->text = "";
      return new WaitingState;
    }
    AdvanceTextIndex(text_, &index_);
    frame_ = (index_ >= text_.size() ? kFinalDelay : kFramesPerGlyph);
  }
  sprite_->battle_->dir = dir_;
  sprite_->battle_->text = text_.substr(0, index_);
  frame_ -= 1;
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
    sprite_->dir_ = GetDirection(move, false /* force_horizontal */);
    move.set_length(kBattleSpeed);
    MoveSprite(game_state, sprite_, &move, &anim_num_);
    return nullptr;
  }
  return new WaitingState;
}

} // namespace battle
} // namespace skishore
