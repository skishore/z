// This file contains SpriteState subclasses that are only used in battles.
#ifndef __SKISHORE_BATTLE_STATE_H__
#define __SKISHORE_BATTLE_STATE_H__

#include "SpriteState.h"

namespace skishore {
namespace battle {

class FaceTargetState : public SpriteState {
 public:
  // The target is a position to face.
  FaceTargetState(const Point& target) : target_(target) {};

  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;

 private:
  Point target_;
};

class WaitingState : public SpriteState {
 public:
  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;
};

class WalkToTargetState : public SpriteState {
 public:
  // The target is a grid square to walk to.
  WalkToTargetState(const Point& target) : target_(target) {};

  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;

 private:
  Point target_;
};

} // namespace battle
} // namespace skishore

#endif  // __SKISHORE_BATTLE_STATE_H__
