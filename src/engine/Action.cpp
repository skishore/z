#include "engine/Action.h"

#include <algorithm>
#include <memory>

#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"
#include "interface/TransliterationShortAnswerGame.h"

using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {

void Action::Bind(Sprite* sprite, GameState* game_state,
                  EventHandler* handler) {
  if (sprite_ != nullptr) {
    ASSERT(dialog_ != nullptr, "Bind was called twice!");
    ASSERT(sprite_ == sprite, "Re-bound with different sprite!");
    return;
  }
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(game_state != nullptr, "game_state == nullptr");
  ASSERT(handler != nullptr, "handler == nullptr");
  sprite_ = sprite;
  game_state_ = game_state;
  handler_ = handler;
}

AttackAction::AttackAction(Sprite* target) : target_(target) {}

ActionResult AttackAction::Execute() {
  ActionResult result;

  if (sprite_->IsPlayer()) {
    if (dialog_ == nullptr) {
      interface::Game* game = new interface::TransliterationShortAnswerGame();
      game_ = game;
      dialog_.reset(game);
      result.stalled = true;
      return result;
    }

    // The player attacks an NPC after playing a semantic game.
    const interface::GameResult game_result = game_->GetResult();
    const int counterattack = game_result.errors;
    const string& enemy = target_->creature.appearance.name;
    if (counterattack > 0) {
      game_state_->log.AddLine("You hit the " + enemy + ".");
      handler_->BeforeAttack(*sprite_, *target_);
      const string followup =
          (counterattack >= sprite_->cur_health ? " You die..." : "");
      game_state_->log.AddLine("The " + enemy + " counters!" + followup);
      handler_->BeforeAttack(*target_, *sprite_);
      sprite_->cur_health = max(sprite_->cur_health - counterattack, 0);
    }
    if (sprite_->IsAlive()) {
      game_state_->log.AddLine("You kill the " + enemy + ".");
      handler_->BeforeAttack(*sprite_, *target_);
      game_state_->RemoveNPC(target_);
    }
    result.success = true;
    return result;
  }

  // An NPC attacks the player. The player will NOT be removed from game state.
  int damage = 0;
  for (int i = 0; i < sprite_->creature.attack.dice; i++) {
    damage += (rand() % sprite_->creature.attack.sides) + 1;
  }
  const string followup = (damage >= target_->cur_health ? " You die..." : "");
  game_state_->log.AddLine(
      "The " + sprite_->creature.appearance.name + " hits!" + followup);
  handler_->BeforeAttack(*sprite_, *target_);
  target_->cur_health = max(target_->cur_health - damage, 0);
  result.success = true;
  return result;
}

MoveAction::MoveAction(const Point& move) : move_(move) {}

ActionResult MoveAction::Execute() {
  ActionResult result;
  Point square = sprite_->square + move_;
  if (game_state_->map.IsSquareBlocked(square)) {
    return result;
  }

  if (square == sprite_->square) {
    result.success = true;
  } else if (game_state_->IsSquareOccupied(square)) {
    result.alternate = new AttackAction(game_state_->SpriteAt(square));
  } else {
    game_state_->MoveSprite(move_, sprite_);
    result.success = true;
  }
  return result;
}

}  // namespace engine
}  // namespace babel
