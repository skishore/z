#include "Sprite.h"
#include "View.h"

using std::vector;

namespace babel {

namespace {

struct TileAppearance {
  char symbol;
  uint32_t lit_color;
  uint32_t dark_color;
};

static const TileAppearance kTileset[] = {
  {'.', 0x005f5f5f, 0x00000000},
  {'.', 0x005f5f5f, 0x00000000},
  {'.', 0x005f5f5f, 0x00000000},
  {'.', 0x005f5f5f, 0x00000000},
  {'#', 0x00907040, 0x00383838},
  {'#', 0x00907040, 0x00383838}
};

static const TileView kUnknownTile = {' ', 0x00000000};

}

View::View(int radius, const GameState& game_state)
    : size(2*radius + 1), tiles(size, vector<TileView>(size)) {
  const Point offset = game_state.player->square - Point(radius, radius);
  for (int x = 0; x < size; x++) {
    for (int y = 0; y < size; y++) {
      Point square = Point(x, y) + offset;
      Tile tile = game_state.map.GetMapTile(square);
      if (game_state.player_vision->IsSquareVisible(square)) {
        tiles[x][y].symbol = kTileset[tile].symbol;
        tiles[x][y].color = kTileset[tile].lit_color;
      } else if (game_state.IsSquareSeen(square)) {
        tiles[x][y].symbol = kTileset[tile].symbol;
        tiles[x][y].color = kTileset[tile].dark_color;
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
      tiles[x][y].symbol = sprite->creature.appearance.symbol;
      tiles[x][y].color = sprite->creature.appearance.color;
    }
  }
}

}  // namespace babel
