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
  DEBUG(sprite->square << " " << move);
  if (IsSquareFree(*game_state, sprite->square + move)) {
    game_state->MoveSprite(sprite, move);
  }
}

}

Engine::Engine() : game_state_("world.dat") {}

const View* Engine::GetView(int radius) const {
  std::unique_ptr<View> view(new View(radius));

  const Point offset = game_state_.player->square - Point(radius, radius);
  for (int x = 0; x <= 2*radius; x++) {
    for (int y = 0; y <= 2*radius; y++) {
      const Point square = Point(x, y) + offset;
      const bool visible = game_state_.player_vision->IsSquareVisible(square);
      const bool blocked = game_state_.map.IsSquareBlocked(square);
      view->tiles[x][y] = (visible ? (blocked ? '#' : '.') : '\0');
    }
  }

  for (auto pair : game_state_.sprites) {
    const Sprite& sprite = *pair.second;
    if (game_state_.player_vision->IsSquareVisible(sprite.square)) {
      const Point& point = sprite.square - offset;
      view->tiles[point.x][point.y] = sprite.creature->appearance.symbol;
    }
  }

  return view.release();
}

bool Engine::HandleCommand(char ch) {
  if (kShift.find(ch) != kShift.end()) {
    const Point& move = kShift.at(ch);
    if (IsSquareFree(game_state_, game_state_.player->square + move)) {
      game_state_.MoveSprite(game_state_.player, move);
      for (auto pair : game_state_.sprites) {
        if (pair.second != game_state_.player) {
          MoveSprite(&game_state_, pair.second);
        }
      }
      return true;
    }
  }
  return false;
}

}  // namespace babel
