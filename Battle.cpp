#include "debug.h"
#include "Battle.h"
#include "GameState.h"

namespace skishore {

namespace {

// SpriteState subclasses that are only used in the Battle mode.
class BattleWaitState : public SpriteState {
 public:
  SpriteState* MaybeTransition(const GameState& game_state) const override {
    return nullptr;
  }

  SpriteState* Update(const GameState& game_state) override {
    sprite_->frame_.x = sprite_->dir_;
    return nullptr;
  }
};

}

Battle::Battle(const GameState& game_state, const Sprite& enemy) {
  ASSERT(enemy.GetRoom() != nullptr, "Started battle with enemy w/o room!");
  player_ = game_state.player_;
  sprites_.push_back(player_);
  for (Sprite* sprite : game_state.sprites_) {
    if (sprite != player_ && sprite->GetRoom() == enemy.GetRoom()) {
      enemies_.push_back(sprite);
      sprites_.push_back(sprite);
    }
  }
  ASSERT(enemies_.size() > 0, "Could not find enemies to battle!");
  for (Sprite* sprite : sprites_) {
    sprite->SetState(new BattleWaitState);
  }
}

bool Battle::Update() {
  return false;
}

} // namespace skishore
