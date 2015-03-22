#include "dialog/DialogAction.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/point.h"
#include "dialog/Dialog.h"
#include "dialog/GraphSearch.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using babel::engine::ActionResult;
using babel::engine::Sprite;
using std::max;
using std::min;
using std::string;
using std::vector;

namespace babel {
namespace dialog {

bool DefendsWithDialog(int damage, const Sprite& sprite) {
  if (sprite.type == 1) {
    return damage >= sprite.cur_health;
  }
  return sprite.type == 4;
}

LaunchDialogAction::LaunchDialogAction(Sprite* target) : target_(target) {}

ActionResult LaunchDialogAction::Execute() {
  ActionResult result;

  ASSERT(sprite_->IsPlayer(), "NPC attack launched a dialog!");

  if (game_state_->dialog != nullptr) {
    // TODO(skishore): Handle this case for complex dialog games.
    ASSERT(false, "Tried to launch a dialog when one was up!");
    return result;
  }

  if (target_->type == 4) {
    game_state_->dialog.reset(
        new TransliterationCombatDialog(sprite_, target_));
    result.stalled = true;
    return result;
  }

  const Point start = target_->square;
  const int target_num_to_spawn = 8;
  vector<Point> squares = GetReachableSquares(
      *game_state_, start, 2*target_num_to_spawn, 4);
  std::random_shuffle(squares.begin(), squares.end());

  game_state_->log.AddLine(
      "You hit the " + target_->creature->appearance.name + ".");
  handler_->OnAttack(sprite_->Id(), target_->Id());


  game_state_->log.AddLine("Its heads roll!");
  target_->Polymorph(2);
  target_->ConsumeEnergy();
  const int num_to_spawn = min(int(squares.size()), target_num_to_spawn);
  for (int i = 0; i < num_to_spawn; i++) {
    Sprite* sprite = new Sprite(squares[i], 3);
    game_state_->AddNPC(sprite);
    sprite->ConsumeEnergy();
  }

  result.success = true;
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
