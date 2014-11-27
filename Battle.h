#ifndef __SKISHORE_BATTLE_H__
#define __SKISHORE_BATTLE_H__

#include "Sprite.h"

namespace skishore {

class GameState;

class Battle {
 public:
  Battle(const GameState& game_state, const Sprite& enemy);

  // Runs the battle through a single time-step. If this method returns true,
  // the battle is over, and this class instance should be destructed.
  bool Update();

 protected:
  void ComputePlaces(std::vector<Point>* places) const;

  const TileMap::Room* room_;

  // Convenience accessors for particular classes of sprite in the battle.
  Sprite* player_;
  std::vector<Sprite*> enemies_;

  // The vector of all sprites in the battle and their places within the room.
  // The places are recorded in grid squares.
  std::vector<Sprite*> sprites_;
  std::vector<Point> places_;
};

} // namespace skishore

#endif  // __SKISHORE_BATTLE_H__
