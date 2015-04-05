#include "dialog/actions.h"

#include <algorithm>

#include "base/point.h"
#include "dialog/dialogs.h"
#include "dialog/graph_search.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using babel::engine::ActionResult;
using babel::engine::GameState;
using babel::engine::Sprite;
using std::max;
using std::string;

namespace babel {
namespace dialog {

bool DefendsWithDialog(const GameState& game_state,
                       const Sprite& sprite, int damage) {
  if (game_state.dialog != nullptr) {
    return game_state.dialog->IsInvolved(sprite);
  }
  return sprite.type == mGecko;
}

LaunchDialogAction::LaunchDialogAction(Sprite* target) : target_(target) {}

ActionResult LaunchDialogAction::Execute() {
  ASSERT(sprite_->IsPlayer());
  ActionResult result;

  if (game_state_->dialog != nullptr) {
    ASSERT(game_state_->dialog->IsInvolved(*target_));
    if (game_state_->dialog->OnAttack(
            game_state_, handler_, sprite_, target_)) {
      game_state_->dialog.reset(nullptr);
    }
    result.success = true;
    return result;
  }

  ASSERT(target_->type == mGecko);
  game_state_->dialog.reset(
      new TransliterationCombatDialog(sprite_, target_));
  result.stalled = true;
  return result;
}

ExecuteCombatAction::ExecuteCombatAction(
    bool complete, int damage, const string& line,
    Sprite* source, Sprite* target)
    : complete_(complete), damage_(damage), line_(line),
      source_(source), target_(target) {}

ActionResult ExecuteCombatAction::Execute() {
  ActionResult result;
  const bool killed = damage_ >= target_->cur_health;

  if (killed && target_->IsPlayer()) {
    line_ += " You die...";
    complete_ = true;
  }
  game_state_->log.AddLine(line_);
  handler_->OnAttack(source_->Id(), target_->Id());

  target_->cur_health = max(target_->cur_health - damage_, 0);
  if (killed && !target_->IsPlayer()) {
    game_state_->RemoveNPC(target_);
  }

  if (complete_) {
    game_state_->dialog.reset(nullptr);
  }
  result.success = complete_;
  result.stalled = !complete_;
  return result;
}

}  // namespace dialog
}  // namespace babel
