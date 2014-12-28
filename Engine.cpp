#include "Engine.h"

#include <memory>

#include "constants.h"
#include "FieldOfVision.h"
#include "Sprite.h"

namespace babel {

namespace {

bool IsSquareFree(const GameState& game_state, const Point& square) {
  return (!game_state.map.IsSquareBlocked(square) &&
          !game_state.IsSquareOccupied(square));
}

}

Engine::Engine() : game_state_("world.dat") {}

bool Engine::HandleCommand(char ch) {
  if (kShift.find(ch) != kShift.end()) {
    const Point& move = kShift.at(ch);
    if ((move.x == 0 && move.y == 0) ||
        IsSquareFree(game_state_, game_state_.player->square + move)) {
      game_state_.MoveSprite(game_state_.player, move);
      for (Sprite* sprite : game_state_.sprites) {
        if (sprite != game_state_.player) {
          sprite->Update(game_state_, this);
        }
      }
      return true;
    }
  }
  return false;
}

void Engine::Attack(Sprite* sprite, Sprite* target) {
  DEBUG("The " << sprite->creature.appearance.name
        << " hits the " << target->creature.appearance.name << ".");
}

void Engine::Move(const Point& move, Sprite* sprite) {
  if (IsSquareFree(game_state_, sprite->square + move)) {
    game_state_.MoveSprite(sprite, move);
  }
}

}  // namespace babel
