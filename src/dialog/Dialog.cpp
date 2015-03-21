#include "dialog/Dialog.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif  // EMSCRIPTEN

#include "dialog/DialogAction.h"

using babel::engine::Action;
using babel::engine::Sprite;

namespace babel {
namespace dialog {

Dialog::~Dialog() {}

TransliterationCombatDialog::TransliterationCombatDialog(
    Sprite* source, Sprite* target) : source_(source), target_(target),
                                      enemy_(target->creature.appearance.name) {
}

Action* TransliterationCombatDialog::OnPageCompletion() {
  return new ExecuteCombatAction(
      true /* complete */, target_->max_health /* damage */,
      "You kill the " + enemy_ + ".", source_, target_);
}

Action* TransliterationCombatDialog::OnTaskCompletion() {
  return new ExecuteCombatAction(
      false /* complete */, 0 /* damage */,
      "You hit the " + enemy_ + ".", source_, target_);
}

Action* TransliterationCombatDialog::OnTaskError() {
  return new ExecuteCombatAction(
      false /* complete */, 1 /* damage */,
      "The " + enemy_ + " hits!", target_, source_);
}

}  // namespace dialog
}  // namespace babel
