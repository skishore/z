#include "dialog/dialogs.h"

#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#define EM_ASM(...)
#define EM_ASM_INT(...) 0
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
namespace {

enum AttackResult {
  WRONG_ENEMY = 0,
  RIGHT_ENEMY = 1,
  COMBAT_WON = 2
};

}  // namespace

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
  sprites_.insert(sprite);
}

int ReverseTransliterationDialog::GetNumEnemies() const {
  return num_enemies_;
}

bool ReverseTransliterationDialog::IsInvolved(const Sprite& sprite) const {
  return sprites_.find(const_cast<engine::Sprite*>(&sprite)) != sprites_.end();
}

bool ReverseTransliterationDialog::OnAttack(
    GameState* game_state, EventHandler* handler,
    Sprite* sprite, Sprite* target) {
  const string& enemy = target->creature->appearance.name;
  const int result = EM_ASM_INT(
      { return DialogManager._current.on_attack($0); }, target->Id());

  if (result == AttackResult::WRONG_ENEMY) {
    game_state->log.AddLine("The " + enemy + " is unfazed.");
    return false;
  }
  game_state->log.AddLine("You destroy the " + enemy + ".");
  handler->OnAttack(sprite->Id(), target->Id());
  game_state->RemoveNPC(target);
  sprites_.erase(target);
  if (result == AttackResult::RIGHT_ENEMY) {
    return false;
  }
  if (sprites_.size() > 0) {
    for (engine::Sprite* left : sprites_) {
      game_state->RemoveNPC(left);
    }
    game_state->log.AddLine("The remainder of the host flees!");
  }
  return true;
}

}  // namespace dialog
}  // namespace babel
