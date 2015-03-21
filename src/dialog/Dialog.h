#ifndef __BABEL_DIALOG_DIALOG_H__
#define __BABEL_DIALOG_DIALOG_H__

#include <string>

#include "engine/Action.h"
#include "engine/Sprite.h"

namespace babel {
namespace dialog {

class Dialog {
 public:
  virtual ~Dialog();

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

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_DIALOG_H__
