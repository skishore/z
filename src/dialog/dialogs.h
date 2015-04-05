#ifndef __BABEL_DIALOG_DIALOGS_H__
#define __BABEL_DIALOG_DIALOGS_H__

#include <map>
#include <set>
#include <string>
#include <vector>

#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

namespace babel {
namespace dialog {

class Dialog {
 public:
  virtual ~Dialog();

  // Returns true if the given sprite is involved in this dialog.
  virtual bool IsInvolved(const engine::Sprite& sprite) const { return false; }

  // Returns a string used to label the given involved monster in the UI.
  virtual std::string GetLabel(
      const engine::Sprite& sprite) const { return ""; }

  // Executes an attack when the target is involved in the dialog.
  // Returns true if the dialog is now over.
  virtual bool OnAttack(
      engine::GameState* game_state, engine::EventHandler* handler,
      engine::Sprite* sprite, engine::Sprite* target) { return false; }

  // Handlers for dialog events that update state and that may return an action
  // to pass to the game engine.
  virtual engine::Action* OnPageCompletion() { return nullptr; }
  virtual engine::Action* OnTaskCompletion() { return nullptr; }
  virtual engine::Action* OnTaskError() { return nullptr; }
};

class TransliterationCombatDialog : public Dialog {
 public:
  TransliterationCombatDialog(engine::Sprite* source, engine::Sprite* target);

  engine::Action* OnPageCompletion() override;
  engine::Action* OnTaskCompletion() override;
  engine::Action* OnTaskError() override;

 private:
  engine::Sprite* source_;
  engine::Sprite* target_;
  const std::string enemy_;
};

class ReverseTransliterationDialog : public Dialog {
 public:
  ReverseTransliterationDialog(const std::vector<engine::Sprite*>& sprites);

  bool IsInvolved(const engine::Sprite& sprite) const override;

  std::string GetLabel(const engine::Sprite& sprite) const override;

  bool OnAttack(
      engine::GameState* game_state, engine::EventHandler* handler,
      engine::Sprite* sprite, engine::Sprite* target) override;

 private:
  std::set<engine::sid> ids_;
  std::map<engine::sid,std::string> text_;
  std::vector<engine::sid> order_;
  int index_ = 0;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_DIALOGS_H__
