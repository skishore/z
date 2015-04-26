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

// The maximum density of enemies spawned by the enemy group trap.
static const double kMaxEnemyDensity = 0.16;

}  // namespace

DialogGroupTrap::DialogGroupTrap(const vector<Point>& squares) {
  for (const Point& square : squares) {
    squares_.push_back(square);
  }
}

void DialogGroupTrap::Trigger(GameState* game_state, EventHandler* handler) {
  // Compute the number of enemies we have room to spawn and check if we can
  // launch a dialog with that many enemies.
  vector<Point> free_squares;
  for (const Point& square : squares_) {
    if (!game_state->map->IsSquareBlocked(square) &&
        !game_state->IsSquareOccupied(square)) {
      free_squares.push_back(square);
    }
  }
  const int max_num_enemies = kMaxEnemyDensity*free_squares.size();
  std::unique_ptr<ReverseTransliterationDialog> dialog(
      new ReverseTransliterationDialog(max_num_enemies));
  const int num_enemies = dialog->GetNumEnemies();
  if (num_enemies == 0) {
    return;
  }

  // Block off the player within the trapped area.
  vector<Point> blocking_squares = GetBlockingSquares(*game_state, squares_);
  for (const Point& square : blocking_squares) {
    game_state->map->SetTile(square, Tile::FENCE);
  }
  game_state->RecomputePlayerVision();
  handler->OnSnapshot();

  // Spawn the new enemies and set the game's dialog to the new state.
  std::random_shuffle(free_squares.begin(), free_squares.end());
  for (int i = 0; i < num_enemies; i++) {
    Sprite* sprite = new Sprite(free_squares[i], mDrone);
    game_state->AddNPC(sprite);
    sprite->ConsumeEnergy();
    dialog->AddEnemy(sprite);
  }
  game_state->log.AddLine("You are ambushed by a group of " +
                          kCreatures[mDrone].appearance.name + "s!");
  game_state->dialog.reset(dialog.release());
}

}  // namespace dialog
}  // namespace babel
