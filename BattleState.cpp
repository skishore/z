#include "constants.h"
#include "BattleData.h"
#include "BattleState.h"

using std::string;

namespace skishore {
namespace battle {

namespace {

const static int kBattleSpeed = 1.2*kPlayerSpeed;
const static int kTolerance = kPlayerSpeed;

// Constants controlling sprite attack animations.
const static int kAttackSpeed = 0.8*kPlayerSpeed;
const static int kAttackFrames = 3;
const static int kAttackAnimNum[] = {4, 4, 16};
const static int kAttackMoveFrames = 8;
const static int kAttackSpriteFrame[][kAttackFrames] =
  {{0, 0, 0}, {8, 1, 9}, {2, 2, 2}, {7, 3, 10}};
const static int kAttackItemFrame[][kAttackFrames] =
  {{0, 0, 0}, {15, 11, 13}, {2, 2, 2}, {12, 14, 16}};
const static Point kAttackItemOffset[][kAttackFrames] =
  {{{}, {}, {}}, {{-1, 2}, {0, 0}, {16, 7}},
   {{}, {}, {}}, {{1, 2}, {-4, -1}, {-21, 6}}};
const static ItemData::ItemStatus kAttackStatus[] =
  {ItemData::BACKGROUND, ItemData::FOREGROUND,
   ItemData::FOREGROUND, ItemData::BACKGROUND};

// Constants controlling sprite damage animations.
const static int kDamageSpeed = 2.4*kPlayerSpeed;
const static int kDamageFrames = 4;

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

SpriteState* AttackState::MaybeTransition(const GameState& game_state) const {
  if (sprite_->dir_ == Direction::UP || sprite_->dir_ == Direction::DOWN ||
      frame_ >= kAttackFrames) {
    sprite_->frame_.x = sprite_->dir_;
    sprite_->item_.status = ItemData::HIDDEN;
    if (sprite_->battle_ == nullptr) {
      return new PausedState;
    }
    return new WaitingState;
  }
  return nullptr;
}

SpriteState* AttackState::Update(const GameState& game_state) {
  if (frame_ == kAttackFrames - 1 && anim_num_ < kAttackMoveFrames) {
    Point move = kShift[sprite_->dir_];
    move.set_length(kAttackSpeed);
    int anim_num = 0;
    MoveSprite(game_state, sprite_, &move, &anim_num);
  }
  if (anim_num_ == 0) {
    Direction dir = sprite_->dir_;
    if (sprite_->is_player_) {
      sprite_->frame_.x = kAttackSpriteFrame[dir][frame_];
      sprite_->item_.frame.x = kAttackItemFrame[dir][frame_];
      sprite_->item_.offset = kAttackItemOffset[dir][frame_];
      sprite_->item_.status = kAttackStatus[dir];
    } else {
      sprite_->frame_.x = 8 + frame_ + (dir == Direction::LEFT ? 3 : 0);
    }
  }
  anim_num_ += 1 + (sprite_->is_player_ ? 1 : 0);
  if (anim_num_ >= kAttackAnimNum[frame_]) {
    anim_num_ = 0;
    frame_ += 1;
  }
  return nullptr;
}

DamageState::DamageState(bool attacked_by_player) {
  anim_num_ = (attacked_by_player ? -4 : -12);
}

SpriteState* DamageState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* DamageState::Update(const GameState& game_state) {
  if (anim_num_ >= 0) {
    Point move = kShift[OppositeDirection(sprite_->dir_)];
    move.set_length(kDamageSpeed);
    int anim_num = 0;
    MoveSprite(game_state, sprite_, &move, &anim_num);
    sprite_->frame_.x = sprite_->dir_;
    if (anim_num_ % 2 == 0) {
      const int offset = (sprite_->is_player_ ? 17 : 14);
      sprite_->frame_.x = (sprite_->frame_.x - 1)/2 + offset;
    }
  }
  anim_num_ += 1;
  if (anim_num_ >= kDamageFrames) {
    sprite_->frame_.x = sprite_->dir_;
    return new WaitingState;
  }
  return nullptr;
}

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
