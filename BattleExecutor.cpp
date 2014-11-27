#include <algorithm>
#include "hungarian.h"

#include "constants.h"
#include "debug.h"
#include "BattleExecutor.h"
#include "GameState.h"

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

void ComputePlaces(const TileMap::Room& room, const vector<Sprite*>& sprites,
                   vector<Point>* places) {
  ASSERT(sprites.size() > 0, "Got an empty sprites list!");
  const int n = sprites.size();
  vector<Point> options(n);
  places->resize(n);

  Point half_square(kGridTicks/2, kGridTicks/2);
  Point position = sprites[0]->GetPosition() + half_square;
  options[0] = ClosestPerimeterSquare(position, room);

  const int perimeter = 2*(room.size.x + room.size.y) - 4;
  for (int i = 1; i < n; i++) {
    const int distance = i*perimeter/n - (i - 1)*perimeter/n;
    options[i] = AdvanceAlongPerimeter(options[i - 1], room, distance);
  }

  vector<vector<Cost>> distances(n, vector<Cost>(n));
  for (int i = 0; i < n; i++) {
    Point position = sprites[i]->GetPosition() + half_square;
    for (int j = 0; j < n; j++) {
      double distance = (position - kGridTicks*options[j]).length();
      distances[i][j] = (int)distance;
    }
  }
  Hungarian hungarian(n, distances);

  for (int i = 0; i < n; i++) {
    (*places)[i] = options[hungarian.GetXMatch(i)];
  }
}

}  // namespace

BattleExecutor::BattleExecutor(
    const TileMap::Room& room, const vector<Sprite*>& sprites)
    : room_(room), sprites_(sprites) {
  ComputePlaces(room_, sprites_, &places_);
}

bool BattleExecutor::Update(const GameState& game_state) {
  return false;
}

}  // namespace battle
}  // namespace skishore
