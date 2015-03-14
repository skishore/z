#ifndef EMSCRIPTEN

#include "ui/Graphics.h"

#include <ncurses.h>

#include "base/util.h"
#include "engine/View.h"

namespace babel {
namespace ui {
namespace {

// The character displayed for each tile and each sprite.
static const char kTileToChar[] = {'.', '.', '.', '.', '#', ' '};
static const char kSpriteToChar[] = {'@', 'x', 'T'};

// Various color indices.
static const int kDefaultColor = 1;
static const int kShadowColor = 2;

}  // namespace

Graphics::Graphics() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, true);
  ESCDELAY = 0;

  start_color();
  use_default_colors();
  init_pair(kDefaultColor, COLOR_BLACK, -1);
  init_pair(kShadowColor, COLOR_WHITE, -1);
}

Graphics::~Graphics() {
  endwin();
}

void Graphics::Redraw(const engine::View& view) {
  for (int y = 0; y < view.size.y; y++) {
    for (int x = 0; x < view.size.x; x++) {
      const engine::TileView& tile = view.tiles[x][y];
      const char ch = (tile.graphic < 0 ? ' ' : kTileToChar[tile.graphic]);
      SetColor(tile.visible ? kDefaultColor : kShadowColor);
      mvaddch(y, x, ch);
    }
  }
  SetColor(kDefaultColor);
  for (int i = 0; i < view.sprites.size(); i++) {
    const engine::SpriteView& sprite = view.sprites[i];
    mvaddch(sprite.square.y, sprite.square.x, kSpriteToChar[sprite.graphic]);
  }
  move(0, view.size.x + 1);
  clrtoeol();
  addstr("skishore the neophyte");
  move(1, view.size.x + 1);
  clrtoeol();
  addstr(("Health: " + IntToString(view.status.cur_health) +
          "/" + IntToString(view.status.max_health)).c_str());
  move(view.size.y + 1, 0);
  clrtobot();
  for (int i = 0; i < view.log.size(); i++) {
    mvaddstr(view.size.y + i + 1, 0, view.log[i].c_str());
  }
  move(view.size.y/2, view.size.x/2);
  refresh();
}

void Graphics::SetColor(int color) {
  if (last_color_ != color) {
    attron(COLOR_PAIR(color));
    last_color_ = color;
  }
}

}  // namespace ui 
}  // namespace babel

#endif  // EMSCRIPTEN
