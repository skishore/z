#include "dialog/traps.h"

#include <algorithm>

#include "dialog/dialogs.h"
#include "dialog/graph_search.h"

using babel::engine::EventHandler;
using babel::engine::GameState;
using babel::engine::Sprite;
using babel::engine::TileMap;
using std::min;
using std::vector;

namespace babel {
namespace dialog {

DialogGroupTrap::DialogGroupTrap(const TileMap::Room& room) {
  for (int x = 0; x < room.size.x; x++) {
    for (int y = 0; y < room.size.y; y++) {
      squares_.push_back(Point(x, y) + room.position);
    }
  }
}

void DialogGroupTrap::Trigger(GameState* game_state, EventHandler* handler) {
  const Point start = game_state->player->square;
  const int target_num_to_spawn = 6;
  vector<Point> squares = GetReachableSquares(
      *game_state, start, 4*target_num_to_spawn, 4); 
  std::random_shuffle(squares.begin(), squares.end());

  game_state->log.AddLine("You are swarmed by a group of " +
                           kCreatures[mDrone].appearance.name + "s!");

  vector<Sprite*> sprites{};
  const int num_to_spawn = min(int(squares.size()), target_num_to_spawn);
  for (int i = 0; i < num_to_spawn; i++) {
    Sprite* sprite = new Sprite(squares[i], mDrone);
    game_state->AddNPC(sprite);
    sprites.push_back(sprite);
    sprite->ConsumeEnergy();
  }

  game_state->dialog.reset(new ReverseTransliterationDialog(sprites));
}

}  // namespace dialog
}  // namespace babel
