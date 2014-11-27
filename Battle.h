#ifndef __SKISHORE_BATTLE_H__
#define __SKISHORE_BATTLE_H__

#include "Sprite.h"
#include "SpriteState.h"

namespace skishore {

class GameState;

class Battle {
 public:
  Battle(const GameState& game_state, const Sprite& enemy);

  // Runs the battle through a single time-step. If this method returns true,
  // the battle is over, and this class instance should be destructed.
  bool Update();

 protected:
  // The list of sprites that are actually in this battle.
  Sprite* player_;
  std::vector<Sprite*> enemies_;
  std::vector<Sprite*> sprites_;
};

} // namespace skishore

#endif  // __SKISHORE_BATTLE_H__
