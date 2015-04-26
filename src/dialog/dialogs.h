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

  // Used to add an enemy to the dialog during its initialization. Does not own.
  virtual void AddEnemy(engine::Sprite* sprite) { ASSERT(false); }

  // Returns the number of enemies needed by this dialog.
  virtual int GetNumEnemies() const { ASSERT(false); return 0; }

  // Returns true if the given sprite is involved in this dialog.
  virtual bool IsInvolved(const engine::Sprite& sprite) const { return false; }

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
  ReverseTransliterationDialog(int max_num_enemies);

  void AddEnemy(engine::Sprite* sprite);
  int GetNumEnemies() const override;
  bool IsInvolved(const engine::Sprite& sprite) const override;

  bool OnAttack(
      engine::GameState* game_state, engine::EventHandler* handler,
      engine::Sprite* sprite, engine::Sprite* target) override;

 private:
  std::set<engine::sid> ids_;
  std::vector<engine::sid> order_;
  int index_ = 0;
  int num_enemies_ = 0;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_DIALOGS_H__
