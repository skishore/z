#include "graphics.h"

#include <assert.h>
#include <ncurses.h>
#include <memory>

#include "constants.h"
#include "engine.h"
#include "view.h"

Graphics::Graphics(Engine* engine) : engine_(engine) {
  assert(engine_);
}

int Graphics::Start() {
  initscr();
  raw();
  noecho();
  keypad(stdscr, true);
  ESCDELAY = 0;

  Redraw();

  while (true) {
    char c = getch();
    if (c == 0x03 || c == 0x1B /* ctrl-C and escape */) {
      break;
    } else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
      if (engine_->HandleCommand(c)) {
        Redraw();
      }
    }
  }

  endwin();
  return 0;
}

void Graphics::Redraw() const {
  std::unique_ptr<const View> view(engine_->GetView());
  assert(view);
  for (int y = 0; y < NROWS; y++) {
    for (int x = 0; x < NCOLS; x++) {
      char tile = view->tiles[x][y];
      mvaddch(y, x, (tile ? tile : '#'));
    }
  }
  const Point& point = view->player_position;
  mvaddch(point.y, point.x, '@');
  move(point.y, point.x);
  refresh();
}
