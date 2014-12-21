#include <algorithm>

#include "debug.h"
#include "Battle.h"
#include "GameState.h"

using std::swap;

namespace skishore {
namespace battle {
namespace {

void RunRandomAttackScript(int n, BattleExecutor* executor) {
  int i = 0;
  int target = (rand() % (n - 1)) + 1;
  if (rand() % 2 == 0) {
    swap(i, target);
  }
  executor->RunScript(
    executor->Attack(i, target)->AndThen(
    executor->AssumePlace(i)->And(executor->AssumePlace(target))));
}

}  // namespace

Battle::Battle(const GameState& game_state, const Sprite& enemy) {
  ASSERT(enemy.GetRoom() != nullptr, "Started battle with enemy w/o room!");
  room_ = enemy.GetRoom();
  player_ = game_state.player_;
  sprites_.push_back(player_);
  int index = 0;
  for (Sprite* sprite : game_state.sprites_) {
    if (sprite != player_ && sprite->GetRoom() == enemy.GetRoom()) {
      if (sprite == &enemy) {
        index = sprites_.size();
      }
      sprites_.push_back(sprite);
    }
  }
  ASSERT(sprites_.size() > 1, "Could not find enemies to battle!");
  ASSERT(index > 0, "Could not find initial enemy!");
  executor_.reset(new BattleExecutor(*room_, sprites_));
  executor_->RunScript(
      executor_->Freeze()->AndThen(
      executor_->Speak(index, "!")->AndThen(
      executor_->AssumePlaces())));
}

bool Battle::Update() {
  if (!executor_->Update()) {
    // We're running a script. Return early.
    return false;
  }
  RunRandomAttackScript(sprites_.size(), executor_.get());
  return false;
}

}  // namespace battle
}  // namespace skishore
