#include <algorithm>
#include <cmath>

#include "constants.h"
#include "debug.h"
#include "BattleData.h"
#include "Sprite.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

namespace {

// Static constraint constants, all in ticks.
static const int kTolerance = 0.2*kGridTicks;
static const int kPushAway = 0.5*kGridTicks;

// Kinematic constraint constants. Can be unitless, in ticks, or in ticks^2.
static const int kKinematicSeparation = 32*kTicksPerPixel;
static const int kKinematicSensitivity = 8*kTicksPerPixel*kTicksPerPixel;
static const double kKinematicMinDist = 2*kTicksPerPixel;
static const double kKinematicPlayerForce  = 1.1;
static const double kKinematicBackoff = 0.4;

// Takes an integer and returns it mod kGridSize, in [0, kGridSize).
inline int gmod(int x) {
  int result = x % kGridTicks;
  return result + (result < 0 ? kGridTicks: 0);
}

// Takes an integer and a divisor and does division and rounding.
inline int divround(int a, int b) {
  return (a + (a > 0 ? b/2 : -b/2))/b;
}

} // namespace

Sprite::Sprite(bool is_player, const Point& square, const Image& image,
               const TileMap& map, const TileMap::Room* room)
    : is_player_(is_player), dir_(Direction::DOWN), image_(image),
      map_(map), room_(room) {
  SetPosition(kGridTicks*square);
  SetState(new PausedState);
}

Sprite::~Sprite() {}

void Sprite::Draw(const Point& camera, const SDL_Rect& bounds,
                  SDL_Surface* surface) const {
  image_.Draw(drawing_position_ - camera, frame_, bounds, surface);
}

SpriteState* Sprite::GetState() const {
  return state_.get();
}

bool Sprite::HasLineOfSight(const Sprite& other) const {
  Point diff = other.square_ - square_;
  Point shift = kShift[dir_];
  if ((shift.x == 0 && diff.x != 0) || (shift.x*diff.x < 0) ||
      (shift.y == 0 && diff.y != 0) || (shift.y*diff.y < 0)) {
    return false;
  }
  Point square = square_;
  while (square != other.square_) {
    square += shift;
    if (!CheckSquare(square)) {
      return false;
    }
  }
  return true;
}

void Sprite::AvoidOthers(const vector<Sprite*> others, Point* move) const {
  if (move->zero()) {
    return;
  }

  Point net;
  for (Sprite* other : others) {
    Point diff = position_ - other->position_;
    if (other->is_player_ || other == this || diff.zero()) {
      continue;
    }
    double length = diff.length();
    if (length < kKinematicSeparation) {
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

bool Sprite::CheckSquare(const Point& square) const {
  if (room_ != nullptr && !room_->Contains(square)) {
    return false;
  }
  return map_.CheckSquare(square);
}

void Sprite::CheckSquares(Point* move) const {
  if (move->zero()) {
    return;
  }

  Point overlap(gmod(position_.x), gmod(position_.y));
  overlap.x -= 2*overlap.x/kGridTicks*kGridTicks;
  overlap.y -= 2*overlap.y/kGridTicks*kGridTicks;

  Point square = position_ - overlap;
  ASSERT(square.x % kGridTicks == 0 && square.y % kGridTicks == 0,
         "Unexpected: " << position_ << " " << overlap << " " << kGridTicks);
  square = square/kGridTicks;

  Point offset;
  bool collided = false;
  double speed = move->length();

  // Check if we cross a horizontal grid boundary going up or down.
  if (move->y < 0 && gmod(position_.y + kTolerance) < -move->y) {
    offset.y = -1;
  } else if (move->y > 0 && gmod(-position_.y) < move->y) {
    offset.y += 1;
  }
  // If we cross a horizontal boundary, check that the next square is open.
  if (offset.y != 0) {
    offset.x = (overlap.x > 0 ? 1 : -1);
    if (!CheckSquare(Point(square.x, square.y + offset.y))) {
      collided = true;
    // Also check for collisions for the square diagonally adjacent to us.
    } else if (abs(overlap.x) > kTolerance && !CheckSquare(square + offset)) {
      collided = true;
      if (abs(overlap.x) < kPushAway && offset.x*move->x <= 0) {
        move->x = -offset.x*speed;
      }
    }
    if (collided) {
      if (offset.y < 0) {
        move->y = kGridTicks - kTolerance - gmod(position_.y);
      } else {
        move->y = gmod(-position_.y);
      }
    }
  }

  // Run similar checks for the velocity x-coordinates.
  offset.x = 0;
  if (move->x < 0 && gmod(position_.x + kTolerance) < -move->x) {
    offset.x = -1;
  } else if (move->x > 0 && gmod(kTolerance - position_.x) < move->x) {
    offset.x = 1;
  }
  if (offset.x != 0) {
    // If we've moved from one square to another in the y direction (that is,
    // if offset.y != 0 && !collided) then run an extra check in the x direction.
    collided = offset.y != 0 && !collided && !CheckSquare(square + offset);
    offset.y = (overlap.y > 0 ? 1 : -1);
    if (!CheckSquare(Point(square.x + offset.x, square.y))) {
      collided = true;
    } else if ((overlap.y > 0 || -overlap.y > kTolerance) &&
               !CheckSquare(square + offset)) {
      collided = true;
      if (abs(overlap.y) < kPushAway && offset.y*move->y <= 0) {
        // Check that we have space to shove in the y direction.
        // We skip this check when shoving in the x direction above because
        // the full x check was after it.
        move->y = (CheckSquare(Point(square.x, square.y + offset.y)) ?
                   -offset.y*speed : 0);
      }
    }
    if (collided) {
      if (offset.x < 0) {
        move->x = kGridTicks - kTolerance - gmod(position_.x);
      } else {
        move->x = kTolerance - gmod(position_.x);
      }
    }
  }
}

void Sprite::SetPosition(const Point& position) {
  position_ = position;
  drawing_position_.x  = divround(position_.x, kTicksPerPixel);
  drawing_position_.y  = divround(position_.y, kTicksPerPixel);
  square_.x  = divround(position_.x, kGridTicks);
  square_.y  = divround(position_.y, kGridTicks);
}

void Sprite::SetState(SpriteState* state) {
  if (state == nullptr) {
    return;
  }
  state_.reset(state);
  state_->Register(this);
}

}  // namespace skishore
