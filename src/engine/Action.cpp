#include <algorithm>
#include <memory>

#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"
#include "semantics/Devanagari.h"

using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {

void Action::Bind(Sprite* sprite, GameState* game_state,
                  vector<EventHandler*>* handlers) {
  ASSERT(sprite_ == nullptr, "Bind was called twice!");
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(game_state != nullptr, "game_state == nullptr");
  ASSERT(handlers != nullptr, "handlers == nullptr");
  sprite_ = sprite;
  game_state_ = game_state;
  handlers_ = handlers;
}

AttackAction::AttackAction(Sprite* target) : target_(target) {}

ActionResult AttackAction::Execute() {
  ActionResult result;
  int damage = 0;
  for (int i = 0; i < sprite_->creature.attack.dice; i++) {
    damage += (rand() % sprite_->creature.attack.sides) + 1;
  }
  bool killed = damage >= target_->cur_health;

  if (sprite_->IsPlayer()) {
    const string verb = (killed ? "kill" : "hit");
    game_state_->log.AddLine(
        "You " + verb + " the " + target_->creature.appearance.name + ".");
  } else if (target_->IsPlayer()) {
    const string followup = (killed ? " You die..." : "");
    game_state_->log.AddLine(
        "The " + sprite_->creature.appearance.name + " hits!" + followup);
  }
  for (EventHandler* handler : *handlers_) {
    handler->BeforeAttack(*sprite_, *target_);
  }

  target_->cur_health = max(target_->cur_health - damage, 0);
  if (killed && !target_->IsPlayer()) {
    game_state_->RemoveNPC(target_);
  }
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
