#include "Engine.h"

#include <memory>

#include "constants.h"
#include "FieldOfVision.h"

namespace babel {

Engine::Engine() {
  map_.LoadMap("world.dat");
  player_position_ = map_.GetStartingSquare();
}

const View* Engine::GetView(int radius) const {
  std::unique_ptr<View> view(new View(radius));

  const FieldOfVision field_of_vision(map_, player_position_, radius);
  const Point offset = player_position_ - Point(radius, radius);
  for (int x = 0; x < NCOLS; x++) {
    for (int y = 0; y < NROWS; y++) {
      const Point square = Point(x, y) + offset;
      const bool visible = field_of_vision.IsSquareVisible(square);
      const bool blocked = map_.IsSquareBlocked(square);
      view->tiles[x][y] = (visible ? (blocked ? 'X' : '.') : '\0');
    }
  }

  for (int i = 0; i < enemy_positions_.size(); i++) {
    if (field_of_vision.IsSquareVisible(enemy_positions_[i])) {
      const Point& point = enemy_positions_[i] - offset;
      view->tiles[point.x][point.y] = (char)((int)'z' - i);
    }
  }

  return view.release();
}

bool Engine::HandleCommand(char ch) {
  bool moved = false;
  if (kShift.find(ch) != kShift.end()) {
    Point new_position = player_position_ + kShift.at(ch);
    if (!map_.IsSquareBlocked(new_position)) {
      player_position_ = new_position;
      if (enemy_positions_.size() > 0 &&
          enemy_positions_.back() == new_position) {
        enemy_positions_.pop_back();
      }
      moved = true;
    }
  }
  if (!moved) {
    return false;
  }
  for (int i = 0; i < enemy_positions_.size(); i++) {
    const Point move = Point((rand() % 3) - 1, (rand() % 3) - 1);
    const Point point = enemy_positions_[i] + move;
    if (!map_.IsSquareBlocked(point)) {
      enemy_positions_[i] = point;
    }
  }
  return true;
}

}  // namespace babel
