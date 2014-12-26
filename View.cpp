#include "Sprite.h"
#include "View.h"

using std::vector;

namespace babel {

namespace {

static const TileView kTileset[] = {
  {'.', 0x005f5f5f},
  {'.', 0x005f5f5f},
  {'.', 0x005f5f5f},
  {'.', 0x005f5f5f},
  {'#', 0x007f7f3f},
  {'#', 0x001f1f1f},
};

static const TileView kUnknownTile = {' ', 0x00000000};

}

View::View(int radius, const GameState& game_state)
    : size(2*radius + 1), tiles(size, vector<TileView>(size)) {
  const Point offset = game_state.player->square - Point(radius, radius);
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      Point square = Point(x, y) + offset;
      if (game_state.player_vision->IsSquareVisible(square)) {
        tiles[x][y] = kTileset[game_state.map.GetMapTile(square)];
      } else {
        tiles[x][y] = kUnknownTile;
      }
    }
  }
  for (const Sprite* sprite : game_state.sprites) {
    int x = sprite->square.x - offset.x;
    int y = sprite->square.y - offset.y;
    if (0 <= x && x < size && 0 <= y && y < size &&
        game_state.player_vision->IsSquareVisible(sprite->square)) {
      tiles[x][y].symbol = sprite->creature->appearance.symbol;
      tiles[x][y].color = sprite->creature->appearance.color;
    }
  }
}

}  // namespace babel
