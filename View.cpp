#include <algorithm>

#include "Sprite.h"
#include "View.h"

using std::max;
using std::vector;

namespace babel {

namespace {
const static int kLogLinesToShow = 4;
}

View::View(int radius, const GameState& game_state)
    : size(2*radius + 1), tiles(size, vector<TileView>(size)) {
  const Point offset = game_state.player->square - Point(radius, radius);
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      Point square = Point(x, y) + offset;
      if (game_state.IsSquareSeen(square)) {
        tiles[x][y].graphic = (int)game_state.map.GetMapTile(square);
        tiles[x][y].visible = game_state.player_vision->IsSquareVisible(square);
      } else {
        tiles[x][y].graphic = -1;
      }
    }
  }
  for (const Sprite* sprite : game_state.sprites) {
    Point square = sprite->square - offset;
    if (0 <= square.x && square.x < size && 0 <= square.y && square.y < size &&
        game_state.player_vision->IsSquareVisible(sprite->square)) {
      const auto& appearance = sprite->creature.appearance;
      sprites.push_back(SpriteView{
          appearance.graphic, appearance.color, square, sprite->text});
    }
  }
  const int log_start = max((int)(game_state.log.size() - kLogLinesToShow), 0);
  for (int i = log_start; i < game_state.log.size(); i++) {
    log.push_back(game_state.log[i]);
  }
}

}  // namespace babel
