#include <algorithm>
#include <cmath>

#include "constants.h"
#include "debug.h"
#include "Sprite.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

namespace {

inline double gmod(double x) {
  double result = fmod(x, kGridSize);
  return result + (result < 0 ? kGridSize : 0);
}

bool CheckSquare(const TileMap& map, const Point& square) {
  return map.GetMapTile(square) != 4;
}

void CheckSquares(const TileMap& map, const Position& pos, Position* move) {
  Point square(pos/kGridSize);
  Point offset;
  bool collided = false;

  double speed = move->length();
  if (speed < kZero) {
    return;
  }

  // Check if we cross a horizontal grid boundary going up or down.
  if (move->y < 0 && gmod(pos.y + kTolerance) < -move->y + kZero) {
    offset.y = -1;
  } else if (move->y > 0 && gmod(pos.y) > kGridSize - move->y - kZero) {
    offset.y += 1;
  }
  // If we cross a horizontal boundary, check that the next square is open.
  if (offset.y != 0) {
    double overlap = pos.x - kGridSize*square.x;
    offset.x = (overlap > 0 ? 1 : -1);
    if (!CheckSquare(map, Point(square.x, square.y + offset.y))) {
      collided = true;
    // Also check for collisions for the square diagonally adjacent to us.
    } else if (abs(overlap) > kTolerance - kZero &&
               !CheckSquare(map, square + offset)) {
      collided = true;
      if (abs(overlap) < kPushAway && offset.x*move->x <= 0) {
        move->x = -offset.x*speed;
      }
    }
    if (collided) {
      if (offset.y < 0) {
        move->y = (kGridSize - kTolerance + kZero) - gmod(pos.y);
      } else {
        move->y = kGridSize - kZero - gmod(pos.y);
      }
    }
  }
}

} // namespace

Sprite::Sprite(bool is_player, const Point& square,
               const Image& image, SpriteState* state)
    : is_player_(is_player), position_(kGridSize*square),
      direction_(Direction::DOWN), image_(image) {
  SetState(state);
}

Point Sprite::GetPosition() const {
  return Point(position_);
}

void Sprite::Draw(const Point& camera, const SDL_Rect& bounds,
                  SDL_Surface* surface) const {
  image_.Draw(Point(position_) - camera, frame_, bounds, surface);
}

SpriteState* Sprite::GetState() {
  return state_.get();
}

void Sprite::SetState(SpriteState* state) {
  ASSERT(state != nullptr, "Got NULL SpriteState!");
  // TODO(skishore): If we add enter/exit methods to states, call them here.
  state_.reset(state);
  state_->Register(this);
}

void Sprite::AvoidOthers(const vector<Sprite*> others, Position* move) const {
  if (move->length() < kZero) {
    return;
  }

  Position net;
  for (Sprite* other : others) {
    if (other->is_player_ || other == this) {
      continue;
    }
    Position diff = position_ - other->position_;
    double length = diff.length();
    if (kZero < length && length < kKinematicSeparation) {
      diff.set_length(kKinematicSensitivity/max(length, kKinematicMinDist));
      if (is_player_) {
        diff.x = (diff.x*move->x > 0 ? 0 : diff.x);
        diff.y = (diff.y*move->y > 0 ? 0 : diff.y);
      }
      net += diff;
    }
  }

  double backoff = (rand() % 2)*kKinematicBackoff;
  if (is_player_) {
    net *= kKinematicPlayerForce;
    backoff = 0;
  }
  if (net.x*move->x < 0) {
    move->x = (abs(move->x) < abs(net.x) ? backoff*net.x : move->x + net.x);
  }
  if (net.y*move->y < 0) {
    move->y = (abs(move->y) < abs(net.y) ? backoff*net.y : move->y + net.y);
  }
}

void Sprite::Move(const TileMap& map, Position* move) {
  CheckSquares(map, position_, move);
  if (move->length() > kZero) {
    position_ += *move;
  }
}

}  // namespace skishore
