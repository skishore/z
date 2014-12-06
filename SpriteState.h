#ifndef __SKISHORE_SPRITE_STATE_H__
#define __SKISHORE_SPRITE_STATE_H__

#include <vector>

#include "Sprite.h"

namespace skishore {

class GameState;

class SpriteState {
 public:
  void Register(Sprite* sprite);

  // In each frame, the game engine calls MaybeTransition for each sprite to
  // check if they are changing state in response to an input event.
  //
  // It then calls Update for each sprite, which can modify the sprite itself
  // and may also cause a state change.
  virtual SpriteState* MaybeTransition(const GameState& game_state) const = 0;
  virtual SpriteState* Update(const GameState& game_state) = 0;

 protected:
  // A pointer to the sprite that owns this state.
  Sprite* sprite_ = nullptr;
  int anim_num_ = 0;
};

class PausedState : public SpriteState {
 public:
  PausedState(int steps=0) : steps_(steps) {};

  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;

 private:
  int steps_;
};

class RandomWalkState : public SpriteState {
 public:
  RandomWalkState(Direction dir, int steps) : dir_(dir), steps_(steps) {};

  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;

 private:
  Direction dir_;
  int steps_;
};

class WalkingState : public SpriteState {
 public:
  SpriteState* MaybeTransition(const GameState& game_state) const override;
  SpriteState* Update(const GameState& game_state) override;
};

SpriteState* MoveSprite(const GameState& game_state, Sprite* sprite,
                        Point* move, int* anim_num);

} // namespace skishore

#endif  // __SKISHORE_SPRITE_STATE_H__
