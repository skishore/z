#include "dialog/traps.h"

#include <algorithm>

#include "dialog/dialogs.h"
#include "dialog/graph_search.h"

using babel::engine::EventHandler;
using babel::engine::GameState;
using babel::engine::Sprite;
using babel::engine::Tile;
using babel::engine::TileMap;
using std::min;
using std::vector;

namespace babel {
namespace dialog {
namespace {

// The maximum number of enemies that will be spawned in a group.
static const int kMaxEnemiesInGroup = 6;

}  // namespace

DialogGroupTrap::DialogGroupTrap(const TileMap::Room& room) {
  for (int x = 0; x < room.size.x; x++) {
    for (int y = 0; y < room.size.y; y++) {
      squares_.push_back(Point(x, y) + room.position);
    }
  }
}

void DialogGroupTrap::Trigger(GameState* game_state, EventHandler* handler) {
  vector<Point> blocking_squares = GetBlockingSquares(*game_state, squares_);
  for (const Point& square : blocking_squares) {
    game_state->map->SetTile(square, Tile::FENCE);
  }
  game_state->RecomputePlayerVision();
  handler->OnSnapshot();

  vector<Point> free_squares;
  for (const Point& square : squares_) {
    if (!game_state->map->IsSquareBlocked(square) &&
        !game_state->IsSquareOccupied(square)) {
      free_squares.push_back(square);
    }
  }
  std::random_shuffle(free_squares.begin(), free_squares.end());

  game_state->log.AddLine("You are ambushed by a group of " +
                          kCreatures[mDrone].appearance.name + "s!");

  const int num_to_spawn = min(int(free_squares.size()), kMaxEnemiesInGroup);
  vector<Sprite*> sprites{};
  for (int i = 0; i < num_to_spawn; i++) {
    Sprite* sprite = new Sprite(free_squares[i], mDrone);
    game_state->AddNPC(sprite);
    sprites.push_back(sprite);
    sprite->ConsumeEnergy();
  }

  game_state->dialog.reset(new ReverseTransliterationDialog(sprites));
}

}  // namespace dialog
}  // namespace babel
