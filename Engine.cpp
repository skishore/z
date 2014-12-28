#include "Engine.h"

#include <algorithm>
#include <memory>

#include "constants.h"
#include "FieldOfVision.h"
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

Engine::Engine() : game_state_("world.dat") {
  game_state_.log.AddLine(
      "Welcome to Babel! You are a neutral male human Padawan.");
  game_state_.log.Coalesce();
}

bool Engine::HandleCommand(char ch) {
  if (!game_state_.player->IsAlive() || kShift.find(ch) == kShift.end()) {
    return false;
  }
  const Point& move = kShift.at(ch);
  const Point& square = game_state_.player->square + move;
  const bool stay = move.x == 0 && move.y == 0;
  if (stay || !game_state_.map.IsSquareBlocked(square)) {
    if (!stay && game_state_.IsSquareOccupied(square)) {
      Attack(game_state_.player, game_state_.SpriteAt(square));
    } else {
      Move(move, game_state_.player);
    }
    for (Sprite* sprite : game_state_.sprites) {
      if (!sprite->IsPlayer()) {
        sprite->Update(game_state_, this);
        if (!game_state_.player->IsAlive()) {
          break;
        }
      }
    }
    game_state_.log.Coalesce();
    return true;
  }
  return false;
}

void Engine::Attack(Sprite* sprite, Sprite* target) {
  int damage = 0;
  for (int i = 0; i < sprite->creature.attack.dice; i++) {
    damage += (rand() % sprite->creature.attack.sides) + 1;
  }
  target->cur_health = max(target->cur_health - damage, 0);
  bool killed = !target->IsAlive();

  if (killed && !target->IsPlayer()) {
    game_state_.RemoveNPC(target);
  }
  if (sprite->IsPlayer()) {
    const string verb = (killed ? "kill" : "hit");
    game_state_.log.AddLine(
        "You " + verb + " the " + target->creature.appearance.name + ".");
  } else if (target->IsPlayer()) {
    const string followup = (killed ? " You die..." : "");
    game_state_.log.AddLine(
        "The " + sprite->creature.appearance.name + " hits!" + followup);
  }
}

void Engine::Move(const Point& move, Sprite* sprite) {
  if (IsSquareFree(game_state_, sprite->square + move)) {
    game_state_.MoveSprite(sprite, move);
  }
}

}  // namespace babel
