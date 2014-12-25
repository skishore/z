#include "Graphics.h"

#include <memory>

#include "constants.h"
#include "debug.h"
#include "Engine.h"
#include "Point.h"
#include "View.h"

namespace babel {

Graphics::Graphics(Engine* engine)
    : engine_(engine), sprite_graphics_(Point(NCOLS, NROWS)) {
  ASSERT(engine_, "engine_ == nullptr");
}

int Graphics::Start() {
  Redraw();

  char c;
  while (input_.GetChar(&c)) {
    if (c == 0x03 || c == 0x1B /* ctrl-C and escape */) {
      break;
    } else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')) {
      if (engine_->HandleCommand(c)) {
        Redraw();
      }
    }
  }
  return 0;
}

void Graphics::Redraw() {
  std::unique_ptr<const View> view(engine_->GetView());
  ASSERT(view, "view == nullptr");
  sprite_graphics_.Clear();
  for (int y = 0; y < NROWS; y++) {
    for (int x = 0; x < NCOLS; x++) {
      if (Point(x, y) == view->player_position) {
        continue;
      }
      char tile = view->tiles[x][y];
      sprite_graphics_.DrawTile(x, y, (tile ? tile : '#'));
    }
  }
  const Point& point = view->player_position;
  // TODO(babel): This is a major hack.
  sprite_graphics_.DrawTile(point.x, point.y, '@');
  for (int y = 0; y < NROWS; y++) {
    for (int x = 0; x < NCOLS; x++) {
      char tile = view->tiles[x][y];
      sprite_graphics_.DrawTileText(x, y, (tile ? tile : '#'));
    }
  }
  sprite_graphics_.Flip();
}

}  // namespace babel
