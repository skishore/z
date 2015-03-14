#ifndef EMSCRIPTEN

#include "ui/Graphics.h"

#include <ncurses.h>

#include "engine/View.h"

namespace babel {
namespace ui {
namespace {

// The character displayed for each tile and each sprite.
static const char kTileToChar[] = {'.', '.', '.', '.', '#', ' '};
static const char kSpriteToChar[] = {'@', 'x', 'T'};

}  // namespace

Graphics::Graphics() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, true);
  ESCDELAY = 0;
}

Graphics::~Graphics() {
  endwin();
}

void Graphics::Redraw(const engine::View& view) {
  for (int y = 0; y < view.size.y; y++) {
    for (int x = 0; x < view.size.x; x++) {
      const engine::TileView& tile = view.tiles[x][y];
      const char ch = (tile.graphic < 0 ? ' ' : kTileToChar[tile.graphic]);
      mvaddch(y, x, ch);
    }
  }
  for (int i = 0; i < view.sprites.size(); i++) {
    const engine::SpriteView& sprite = view.sprites[i];
    mvaddch(sprite.square.y, sprite.square.x, kSpriteToChar[sprite.graphic]);
  }
  move(view.size.y/2, view.size.x/2);
  refresh();
}

}  // namespace ui 
}  // namespace babel

#endif  // EMSCRIPTEN
