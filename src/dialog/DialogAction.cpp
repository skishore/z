#include "dialog/DialogAction.h"

#include <algorithm>
#include <string>

#include "dialog/Dialog.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using babel::engine::ActionResult;
using babel::engine::Sprite;
using std::max;
using std::string;

namespace babel {
namespace dialog {

bool DefendsWithDialog(const Sprite& sprite) {
  return !sprite.IsPlayer();
}

LaunchDialogAction::LaunchDialogAction(Sprite* target) : target_(target) {}

ActionResult LaunchDialogAction::Execute() {
  ActionResult result;

  if (game_state_->dialog != nullptr) {
    // TODO(skishore): Handle this case for complex dialog games.
    ASSERT(false, "Tried to launch a dialog when one was up!");
    return result;
  }

  game_state_->dialog.reset(new TransliterationCombatDialog(target_));
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
  }
  game_state_->log.AddLine(line_);
  handler_->BeforeAttack(source_->Id(), target_->Id());

  target_->cur_health = max(target_->cur_health - damage_, 0);
  if (killed && !target_->IsPlayer()) {
    game_state_->RemoveNPC(target_);
  }

  if (complete_) {
    game_state_->dialog.reset(nullptr);
  }
  result.stalled = !complete_;
  return result;
}

}  // namespace dialog
}  // namespace babel
