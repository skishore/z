#include <algorithm>
#include <memory>

#include "Action.h"
#include "GameState.h"
#include "Sprite.h"

using std::max;
using std::string;

namespace babel {

namespace {

bool IsSquareFree(const GameState& game_state, const Point& square) {
  return (!game_state.map.IsSquareBlocked(square) &&
          !game_state.IsSquareOccupied(square));
}

}

void Action::Bind(Sprite* sprite) {
  ASSERT(sprite_ == nullptr, "Bind was called twice!");
  ASSERT(sprite != nullptr, "sprite == nullptr");
  sprite_ = sprite;
}

AttackAction::AttackAction(Sprite* target) : target_(target) {}

void AttackAction::Execute(GameState* game_state) {
  int damage = 0;
  for (int i = 0; i < sprite_->creature.attack.dice; i++) {
    damage += (rand() % sprite_->creature.attack.sides) + 1;
  }
  target_->cur_health = max(target_->cur_health - damage, 0);
  bool killed = !target_->IsAlive();

  if (sprite_->IsPlayer()) {
    const string verb = (killed ? "kill" : "hit");
    game_state->log.AddLine(
        "You " + verb + " the " + target_->creature.appearance.name + ".");
  } else if (target_->IsPlayer()) {
    const string followup = (killed ? " You die..." : "");
    game_state->log.AddLine(
        "The " + sprite_->creature.appearance.name + " hits!" + followup);
  }
  if (killed && !target_->IsPlayer()) {
    game_state->RemoveNPC(target_);
  }
}

MoveAction::MoveAction(const Point& move) : move_(move) {}

void MoveAction::Execute(GameState* game_state) {
  if (IsSquareFree(*game_state, sprite_->square + move_)) {
    game_state->MoveSprite(move_, sprite_);
  }
}

}  // namespace babel
