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

void MoveSprite(GameState* game_state, Sprite* sprite) {
  const Point move = sprite->GetMove(*game_state);
  if (IsSquareFree(*game_state, sprite->square + move)) {
    game_state->MoveSprite(sprite, move);
  }
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
          MoveSprite(&game_state_, sprite);
        }
      }
      return true;
    }
  }
  return false;
}

}  // namespace babel
