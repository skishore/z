#ifndef __BABEL_DIALOG_ACTIONS_H__
#define __BABEL_DIALOG_ACTIONS_H__

#include <string>

#include "engine/Action.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

namespace babel {
namespace dialog {

// Returns true if attacking the given target launches a dialog.
bool DefendsWithDialog(const engine::GameState& game_state,
                       const engine::Sprite& target, int damage);

class DialogAction : public engine::Action {
 public:
  // DialogActions are queueable so that signals from the UI are never dropped.
  bool Queueable() override { return true; }
};

class LaunchDialogAction : public DialogAction {
 public:
  LaunchDialogAction(engine::Sprite* target);
  engine::ActionResult Execute() override;

 private:
  engine::Sprite* target_;
};

class ExecuteCombatAction : public DialogAction {
 public:
  ExecuteCombatAction(bool complete, int damage, const std::string& line,
                      engine::Sprite* source, engine::Sprite* target);
  engine::ActionResult Execute() override;

 private:
  bool complete_;
  int damage_;
  std::string line_;
  engine::Sprite* source_;
  engine::Sprite* target_;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_ACTIONS_H__
