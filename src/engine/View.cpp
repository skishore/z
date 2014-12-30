#include <algorithm>

#include "engine/Sprite.h"
#include "engine/View.h"

using std::max;
using std::vector;

namespace babel {
namespace engine {

View::View(int radius, const GameState& game_state)
    : size(2*radius + 1), tiles(size, vector<TileView>(size)) {
  const Point offset = game_state.player->square - Point(radius, radius);
  const int vision = game_state.player->creature.stats.vision_radius;
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      Point square = Point(x, y) + offset;
      if (game_state.IsSquareSeen(square)) {
        tiles[x][y].graphic = (int)game_state.map.GetMapTile(square);
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
    if (0 <= square.x && square.x < size && 0 <= square.y && square.y < size &&
        game_state.player_vision->IsSquareVisible(sprite->square, vision)) {
      const auto& appearance = sprite->creature.appearance;
      sprites.push_back(SpriteView{
          appearance.graphic, appearance.color, square, sprite->text});
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
