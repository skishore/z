#include <algorithm>

#include "constants.h"
#include "debug.h"
#include "Battle.h"
#include "BattleState.h"
#include "GameState.h"
#include "SpriteState.h"

using std::min;
using std::max;
using std::vector;

namespace skishore {
namespace battle {

namespace {

// Takes an integer and a divisor and does division and rounding.
inline int divround(int a, int b) {
    return (a + (a > 0 ? b/2 : -b/2))/b;
}

// Takes a position and returns the square on the perimeter of the given room
// that is closest to that position.
Point ClosestPerimeterSquare(const Point& position, const TileMap::Room& room) {
  Point result = kGridTicks*room.position;
  if (position.x > result.x + kGridTicks*room.size.x/2) {
    result.x += kGridTicks*room.size.x;
  }
  if (position.y > result.y + kGridTicks*room.size.y/2) {
    result.y += kGridTicks*room.size.y;
  }
  if (abs(result.x - position.x) > abs(result.y - position.y)) {
    result.x = position.x;
  } else {
    result.y = position.y;
  }
  result.x = divround(result.x, kGridTicks);
  result.y = divround(result.y, kGridTicks);
  Point other_corner = room.position + room.size - Point(1, 1);
  result.x = min(max(room.position.x, result.x), other_corner.x);
  result.y = min(max(room.position.y, result.y), other_corner.y);
  return result;
}

// Advance distance steps along the room's perimeter in the ccw direction.
Point AdvanceAlongPerimeter(
    const Point& square, const TileMap::Room& room, int distance) {
  if (room.size.x <= 1 || room.size.y <= 1) {
    return square;
  }
  Point result = square;
  while (distance > 0) {
    if (result.x == room.position.x) {
      int new_y = max(result.y - distance, room.position.y);
      distance -= result.y - new_y;
      result.y = new_y;
    } else if (result.x == room.position.x + room.size.x - 1) {
      int new_y = min(result.y + distance, room.position.y + room.size.y - 1);
      distance -= new_y - result.y;
      result.y = new_y;
    }
    if (result.y == room.position.y) {
      int new_x = min(result.x + distance, room.position.x + room.size.x - 1);
      distance -= new_x - result.x;
      result.x = new_x;
    } else if (result.y == room.position.y + room.size.y - 1) {
      int new_x = max(result.x - distance, room.position.x);
      distance -= result.x - new_x;
      result.x = new_x;
    }
  }
  return result;
}

}  // namespace

Battle::Battle(const GameState& game_state, const Sprite& enemy) {
  ASSERT(enemy.GetRoom() != nullptr, "Started battle with enemy w/o room!");
  room_ = enemy.GetRoom();
  player_ = game_state.player_;
  sprites_.push_back(player_);
  for (Sprite* sprite : game_state.sprites_) {
    if (sprite != player_ && sprite->GetRoom() == enemy.GetRoom()) {
      enemies_.push_back(sprite);
      sprites_.push_back(sprite);
    }
  }
  ASSERT(enemies_.size() > 0, "Could not find enemies to battle!");
  ComputePlaces(&places_);
  for (int i = 0; i < sprites_.size(); i++) {
    sprites_[i]->SetState(new WalkToTargetState(places_[i]));
  }
}

void Battle::ComputePlaces(vector<Point>* places) const {
  ASSERT(sprites_.size() > 0 && sprites_[0] == player_, "Unexpected sprites!");
  const int n = sprites_.size();
  vector<Point> options(n);
  places->resize(n);

  Point half_square(kGridTicks/2, kGridTicks/2);
  Point position = player_->GetPosition() + half_square;
  options[0] = ClosestPerimeterSquare(position, *room_);

  const int perimeter = 2*(room_->size.x + room_->size.y) - 4;
  for (int i = 1; i < n; i++) {
    const int distance = i*perimeter/n - (i - 1)*perimeter/n;
    options[i] = AdvanceAlongPerimeter(options[i - 1], *room_, distance);
  }

  vector<bool> assigned(n, false);
  for (int i = 0; i < n; i++) {
    Point position = sprites_[i]->GetPosition() + half_square;
    double best_distance = kGridTicks*perimeter;
    int best_index = -1;
    for (int j = 0; j < n; j++) {
      if (assigned[j]) {
        continue;
      }
      double distance = (position - kGridTicks*options[j]).length();
      if (distance < best_distance) {
        best_distance = distance;
        best_index = j;
      }
    }
    ASSERT(best_index >= 0, "Failed to find good option for " << i);
    (*places)[i] = options[best_index];
    assigned[best_index] = true;
  }
}

bool Battle::Update() {
  return false;
}

}  // namespace battle
}  // namespace skishore
