#ifndef EMSCRIPTEN

#include "ui/Graphics.h"

#include <ncurses.h>

#include "base/util.h"
#include "engine/View.h"

namespace babel {
namespace ui {
namespace {

struct Glyph {
  char ch;
  int color;
};

// The glyph displayed for each tile.
static const Glyph kTileGlyphs[][6] = {
  // Glyphs for hidden tiles.
  {{' ', COLOR_BLACK},
   {' ', COLOR_BLACK},
   {' ', COLOR_BLACK},
   {' ', COLOR_BLACK},
   {'#', COLOR_WHITE},
   {' ', COLOR_BLACK}},
  // Glyphs for visible tiles.
  {{'.', COLOR_BLACK},
   {'.', COLOR_BLACK},
   {'.', COLOR_BLACK},
   {'.', COLOR_BLACK},
   {'#', COLOR_BLACK},
   {' ', COLOR_BLACK}}};

// The glyph displayed for each sprite.
static const Glyph kSpriteGlyphs[] = {
  {'@', COLOR_WHITE},
  {'b', COLOR_GREEN},
  {'c', COLOR_RED},
  {'s', COLOR_CYAN},
  {'r', COLOR_YELLOW},
  {'p', COLOR_BLACK}};

}  // namespace

Graphics::Graphics() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, true);
  ESCDELAY = 0;

  start_color();
  use_default_colors();
  for (int i = 0; i < 16; i++) {
    init_pair(i + 1, i, -1);
  }
}

Graphics::~Graphics() {
  endwin();
}

void Graphics::Redraw(const engine::View& view) {
  // Draw the tiles.
  for (int y = 0; y < view.size.y; y++) {
    move(y, 0);
    for (int x = 0; x < view.size.x; x++) {
      const engine::TileView& tile = view.tiles[x][y];
      if (tile.graphic >= 0) {
        const Glyph& glyph = kTileGlyphs[tile.visible][tile.graphic];
        SetColor(glyph.color);
        addch(glyph.ch);
      } else {
        addch(' ');
      }
    }
  }
  // Draw the sprites.
  for (int i = 0; i < view.sprites.size(); i++) {
    const engine::SpriteView& sprite = view.sprites[i];
    const Glyph& glyph = kSpriteGlyphs[sprite.graphic];
    SetColor(glyph.color);
    mvaddch(sprite.square.y, sprite.square.x, glyph.ch);
  }
  // Draw the status UI to the right.
  SetColor(COLOR_BLACK);
  move(0, view.size.x + 1);
  clrtoeol();
  addstr("skishore the neophyte");
  move(1, view.size.x + 1);
  clrtoeol();
  addstr(("Health: " + IntToString(view.status.cur_health) +
          "/" + IntToString(view.status.max_health)).c_str());
  // Draw the log at the bottom.
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
    attron(COLOR_PAIR(color + 1));
    last_color_ = color;
  }
}

}  // namespace ui 
}  // namespace babel

#endif  // EMSCRIPTEN
