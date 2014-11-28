#ifndef __SKISHORE_BATTLE_EXECUTOR_H__
#define __SKISHORE_BATTLE_EXECUTOR_H__

#include "Sprite.h"
#include "TileMap.h"

namespace skishore {

namespace battle {

class BattleStep;

class BattleExecutor {
 public:
  BattleExecutor(const TileMap::Room& room,
                 const std::vector<Sprite*>& sprites);

  // Moves sprite i to its place, or moves all sprites to their places.
  void WalkToPlace(int i);
  void WalkToPlaces();

  // Runs the script through a single time-step. If this method returns true,
  // then all current scripts have finished.
  bool Update();

 protected:
  const TileMap::Room& room_;
  const std::vector<Sprite*>& sprites_;
  std::vector<Point> places_;
};

}  // namespace battle
}  // namespace skishore

#endif  // __SKISHORE_BATTLE_EXECUTOR_H__
