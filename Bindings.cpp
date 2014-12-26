#include "Bindings.h"

#include <memory>

#include "constants.h"
#include "debug.h"
#include "Engine.h"
#include "Point.h"
#include "View.h"

namespace babel {

Bindings::Bindings(Engine* engine)
    : engine_(engine), graphics_(Point(NCOLS, NROWS)) {
  ASSERT(engine_, "engine_ == nullptr");
}

int Bindings::Start() {
  Redraw();

  char ch;
  while (input_.GetChar(&ch)) {
    if (ch == 0x03 || ch == 0x1B /* ctrl-C and escape */) {
      break;
    } else if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')) {
      if (engine_->HandleCommand(ch)) {
        Redraw();
      }
    }
  }
  return 0;
}

void Bindings::Redraw() {
  std::unique_ptr<const View> view(engine_->GetView(kVisionRadius));
  ASSERT(view, "view == nullptr");
  graphics_.Clear();
  for (int y = 0; y < NROWS; y++) {
    for (int x = 0; x < NCOLS; x++) {
      if (Point(x, y) == view->player_position) {
        continue;
      }
      char tile = view->tiles[x][y];
      graphics_.DrawTile(x, y, (tile ? tile : '#'));
    }
  }
  const Point& point = view->player_position;
  // TODO(babel): This is a major hack.
  graphics_.DrawTile(point.x, point.y, '@');
  for (int y = 0; y < NROWS; y++) {
    for (int x = 0; x < NCOLS; x++) {
      char tile = view->tiles[x][y];
      graphics_.DrawTileText(x, y, (tile ? tile : '#'));
    }
  }
  graphics_.Flip();
}

}  // namespace babel
