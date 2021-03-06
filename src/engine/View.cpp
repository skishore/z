#include "engine/View.h"

#include <algorithm>

#include "dialog/dialogs.h"

using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {

View::View(const Point& s, const GameState& game_state)
    : size(s), offset(0, 0), tiles(size.x, vector<TileView>(size.y)) {
  const int vision = game_state.player->creature->stats.vision_radius;
  for (int x = 0; x < size.x; x++) {
    for (int y = 0; y < size.y; y++) {
      Point square = Point(x, y) + offset;
      if (game_state.IsSquareSeen(square)) {
        tiles[x][y].graphic = game_state.map->GetGraphic(square);
        tiles[x][y].visible =
            game_state.player_vision->IsSquareVisible(square, vision);
      } else {
        tiles[x][y].graphic = -1;
      }
    }
  }
  for (const Sprite* sprite : game_state.sprites) {
    if (!sprite->IsAlive()) {
      continue;
    }
    Point square = sprite->square - offset;
    if (0 <= square.x && square.x < size.x &&
        0 <= square.y && square.y < size.y &&
        game_state.player_vision->IsSquareVisible(sprite->square, vision)) {
      const auto& appearance = sprite->creature->appearance;
      sprites.push_back(SpriteView{sprite->Id(), appearance.graphic, square});
    }
  }
  if (game_state.log.IsFresh()) {
    log = game_state.log.GetLastLines(1);
  }
  status.cur_health = game_state.player->cur_health;
  status.max_health = game_state.player->max_health;
}

}  // namespace engine 
}  // namespace babel
