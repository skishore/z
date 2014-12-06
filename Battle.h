#ifndef __SKISHORE_BATTLE_H__
#define __SKISHORE_BATTLE_H__

#include <memory>

#include "BattleExecutor.h"
#include "Sprite.h"

namespace skishore {

class GameState;

namespace battle {

class Battle {
 public:
  Battle(const GameState& game_state, const Sprite& enemy);

  // Used by GameState to center the camera during a battle.
  const Point& GetCenter() const { return executor_->GetCenter(); }

  // Runs the battle through a single time-step. If this method returns true,
  // the battle is over, and this class instance should be destructed.
  bool Update();

 private:
  const TileMap::Room* room_;
  std::unique_ptr<BattleExecutor> executor_;

  // The player sprite and the list of all sprites in the battle.
  // The player's sprite will ALWAYS be first in the list.
  Sprite* player_;
  std::vector<Sprite*> sprites_;
};

}  // namespace battle
}  // namespace skishore

#endif  // __SKISHORE_BATTLE_H__
