#include "Engine.h"

#include <memory>

#include "constants.h"
#include "FieldOfVision.h"

namespace skishore {

namespace {
char GetRandomTile() {
  return (rand() % 8 == 0 ? 'X' : '.');
}
}

Engine::Engine()
    : tiles_(NCOLS, NROWS), player_position_((NCOLS + 1)/2, (NROWS + 1)/2) {
  for (int x = 0; x < NCOLS; x++) {
    for (int y = 0; y < NROWS; y++) {
      tiles_.tiles[x][y] = GetRandomTile();
    }
  }
}

const View* Engine::GetView() const {
  std::unique_ptr<View> view(new View);

  const FieldOfVision field_of_vision(tiles_, player_position_);
  for (int x = 0; x < NCOLS; x++) {
    for (int y = 0; y < NROWS; y++) {
      const bool visible = field_of_vision.IsSquareVisible(x, y);
      view->tiles[x][y] = (visible ? tiles_.tiles[x][y] : '\0');
    }
  }

  view->player_position = player_position_;
  return view.release();
}

bool Engine::HandleCommand(char c) {
  Point point = player_position_;
  // TODO(skishore): Replace this code with a lookup table.
  if (c == 'h') {
    point.x -= 1;
  } else if (c == 'j') {
    point.y += 1;
  } else if (c == 'k') {
    point.y -= 1;
  } else if (c == 'l') {
    point.x += 1;
  } else if (c == 'y') {
    point.x -= 1;
    point.y -= 1;
  } else if (c == 'u') {
    point.x += 1;
    point.y -= 1;
  } else if (c == 'b') {
    point.x -= 1;
    point.y += 1;
  } else if (c == 'n') {
    point.x += 1;
    point.y += 1;
  }
  if (point.x != player_position_.x || point.y != player_position_.y) {
    if (!tiles_.IsSquareBlocked(point.x, point.y)) {
      player_position_ = point;
      return true;
    }
  }
  return false;
}

}  // namespace skishore
