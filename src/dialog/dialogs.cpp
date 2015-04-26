#include "dialog/dialogs.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EM_ASM(...)
#define EM_ASM_INT(...)
#endif  // EMSCRIPTEN

#include "dialog/actions.h"

using babel::engine::Action;
using babel::engine::EventHandler;
using babel::engine::GameState;
using babel::engine::Sprite;
using std::min;
using std::set;
using std::string;
using std::vector;

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

ReverseTransliterationDialog::ReverseTransliterationDialog(
    int max_num_enemies) {
  num_enemies_ = EM_ASM_INT({
    var dialog = new EnglishToHindiMultipleChoiceGame($0);
    var num_enemies = dialog.get_num_enemies();
    if (num_enemies > 0) {
      DialogManager.set_text('Fight off the drones by transliterating:');
      DialogManager.set_page(dialog);
    }
    return num_enemies;
  }, max_num_enemies);
}

void ReverseTransliterationDialog::AddEnemy(engine::Sprite* sprite) {
  EM_ASM_INT({ DialogManager._current.add_enemy($0); }, sprite->Id());
  ids_.insert(sprite->Id());
  order_.push_back(sprite->Id());
}

int ReverseTransliterationDialog::GetNumEnemies() const {
  return num_enemies_;
}

bool ReverseTransliterationDialog::IsInvolved(const Sprite& sprite) const {
  return ids_.find(sprite.Id()) != ids_.end();
}

bool ReverseTransliterationDialog::OnAttack(
    GameState* game_state, EventHandler* handler,
    Sprite* sprite, Sprite* target) {
  const string& enemy = target->creature->appearance.name;
  if (target->Id() != order_[index_]) {
    game_state->log.AddLine("The " + enemy + " is unfazed.");
    return false;
  }
  game_state->log.AddLine("You destroy the " + enemy + ".");
  handler->OnAttack(sprite->Id(), target->Id());
  game_state->RemoveNPC(target);
  index_ += 1;
  return index_ == order_.size();
}

}  // namespace dialog
}  // namespace babel
