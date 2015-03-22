#include "dialog/Dialog.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EM_ASM(...)
#define EM_ASM_INT(...)
#endif  // EMSCRIPTEN

#include "dialog/DialogAction.h"

using babel::engine::Action;
using babel::engine::Sprite;

namespace babel {
namespace dialog {

Dialog::~Dialog() {
  EM_ASM(DialogManager.reset());
}

TransliterationCombatDialog::TransliterationCombatDialog(
    Sprite* source, Sprite* target)
    : source_(source), target_(target),
      enemy_(target->creature->appearance.name) {
  EM_ASM_INT({
    DialogManager.set_text('The ' + Module.Pointer_stringify($0) +
                           ' is vulnerable to transliteration:');
    DialogManager.set_page(new HindiToEnglishShortAnswerGame());
  }, enemy_.c_str());
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
