#include <algorithm>
#include <memory>

#include "engine/Action.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using std::max;
using std::string;

namespace babel {
namespace engine {

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

Action* AttackAction::Execute(GameState* game_state, bool* success) {
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
  *success = true;
  return nullptr;
}

MoveAction::MoveAction(const Point& move) : move_(move) {}

Action* MoveAction::Execute(GameState* game_state, bool* success) {
  Point square = sprite_->square + move_;
  if (square == sprite_->square) {
    *success = true;
  } else if (IsSquareFree(*game_state, square)) {
    game_state->MoveSprite(move_, sprite_);
    *success = true;
  } else if (game_state->IsSquareOccupied(square)) {
    return new AttackAction(game_state->SpriteAt(square));
  }
  return nullptr;
}

}  // namespace engine
}  // namespace babel
