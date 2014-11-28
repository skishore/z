#include "debug.h"
#include "Battle.h"
#include "GameState.h"

namespace skishore {
namespace battle {

Battle::Battle(const GameState& game_state, const Sprite& enemy) {
  ASSERT(enemy.GetRoom() != nullptr, "Started battle with enemy w/o room!");
  room_ = enemy.GetRoom();
  player_ = game_state.player_;
  sprites_.push_back(player_);
  for (Sprite* sprite : game_state.sprites_) {
    if (sprite != player_ && sprite->GetRoom() == enemy.GetRoom()) {
      sprites_.push_back(sprite);
    }
  }
  ASSERT(sprites_.size() > 1, "Could not find enemies to battle!");
  executor_.reset(new BattleExecutor(*room_, sprites_));
  executor_->WalkToPlaces();
}

bool Battle::Update() {
  return false;
}

}  // namespace battle
}  // namespace skishore
