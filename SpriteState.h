#ifndef __SKISHORE_SPRITE_STATE_H__
#define __SKISHORE_SPRITE_STATE_H__

#include "Sprite.h"

namespace skishore {

class GameState;

class SpriteState {
 public:
  void Register(Sprite* sprite);

  // Returns true if the sprite can be hit by attacks in this state.
  virtual bool Hittable() const { return true; };

  // In each frame, the game engine calls MaybeTransition for each sprite to
  // check if they are changing state, then Move to get a position delta.
  // Either of these methods may return NULL, which indicates no change.
  virtual SpriteState* MaybeTransition(const GameState& game_state) = 0;
  virtual Position* Move(const GameState& game_state) = 0;

 protected:
  // A pointer to the sprite that owns this state.
  Sprite* self_ = nullptr;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_STATE_H__
