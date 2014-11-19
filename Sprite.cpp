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

// Returns true if the given square is free.
bool CheckSquare(const TileMap& map, const Point& square) {
  return map.GetMapTile(square) != 4;
}

// Takes a move and applies static map constraints to it.
void CheckSquares(const TileMap& map, const Point& pos, Point* move) {
  if (move->x == 0 && move->y == 0) {
    return;
  }

  Point overlap(gmod(pos.x), gmod(pos.y));
  overlap.x -= 2*overlap.x/kGridTicks*kGridTicks;
  overlap.y -= 2*overlap.y/kGridTicks*kGridTicks;

  Point square = pos - overlap;
  ASSERT(square.x % kGridTicks == 0 && square.y % kGridTicks == 0,
         "Unexpected: " << pos << " " << overlap << " " << " " << kGridTicks);
  square = square/kGridTicks;

  Point offset;
  bool collided = false;
  double speed = move->length();

  // Check if we cross a horizontal grid boundary going up or down.
  if (move->y < 0 && gmod(pos.y + kTolerance) < -move->y) {
    offset.y = -1;
  } else if (move->y > 0 && gmod(-pos.y) < move->y) {
    offset.y += 1;
  }
  // If we cross a horizontal boundary, check that the next square is open.
  if (offset.y != 0) {
    offset.x = (overlap.x > 0 ? 1 : -1);
    if (!CheckSquare(map, Point(square.x, square.y + offset.y))) {
      collided = true;
    // Also check for collisions for the square diagonally adjacent to us.
    } else if (abs(overlap.x) > kTolerance &&
               !CheckSquare(map, square + offset)) {
      collided = true;
      if (abs(overlap.x) < kPushAway && offset.x*move->x <= 0) {
        move->x = -offset.x*speed;
      }
    }
    if (collided) {
      if (offset.y < 0) {
        move->y = kGridTicks - kTolerance - gmod(pos.y);
      } else {
        move->y = gmod(-pos.y);
      }
    }
  }

  // Run similar checks for the velocity x-coordinates.
  offset.x = 0;
  if (move->x < 0 && gmod(pos.x + kTolerance) < -move->x) {
    offset.x = -1;
  } else if (move->x > 0 && gmod(kTolerance - pos.x) < move->x) {
    offset.x = 1;
  }
  if (offset.x != 0) {
    // If we've moved from one square to another in the y direction (that is,
    // if offset.y != 0 && !collided) then run an extra check in the x direction.
    collided = offset.y != 0 && !collided && !CheckSquare(map, square + offset);
    offset.y = (overlap.y > 0 ? 1 : -1);
    if (!CheckSquare(map, Point(square.x + offset.x, square.y))) {
      collided = true;
    } else if ((overlap.y > 0 || -overlap.y > kTolerance) &&
               !CheckSquare(map, square + offset)) {
      collided = true;
      if (abs(overlap.y) < kPushAway && offset.y*move->y <= 0) {
        // Check that we have space to shove in the y direction.
        // We skip this check when shoving in the x direction above because
        // the full x check was after it.
        move->y = (CheckSquare(map, Point(square.x, square.y + offset.y)) ?
                   -offset.y*speed : 0);
      }
    }
    if (collided) {
      if (offset.x < 0) {
        move->x = kGridTicks - kTolerance - gmod(pos.x);
      } else {
        move->x = kTolerance - gmod(pos.x);
      }
    }
  }
}

} // namespace

Sprite::Sprite(bool is_player, const Point& square,
               const Image& image, SpriteState* state)
    : is_player_(is_player), direction_(Direction::DOWN), image_(image) {
  SetPosition(kGridTicks*square);
  SetState(state);
}

void Sprite::Draw(const Point& camera, const SDL_Rect& bounds,
                  SDL_Surface* surface) const {
  image_.Draw(drawing_position_ - camera, frame_, bounds, surface);
}

SpriteState* Sprite::GetState() const {
  return state_.get();
}

void Sprite::SetState(SpriteState* state) {
  ASSERT(state != nullptr, "Got NULL SpriteState!");
  // TODO(skishore): If we add enter/exit methods to states, call them here.
  state_.reset(state);
  state_->Register(this);
}

void Sprite::AvoidOthers(const vector<Sprite*> others, Point* move) const {
  if (move->x == 0 && move->y == 0) {
    return;
  }

  Point net;
  for (Sprite* other : others) {
    if (other->is_player_ || other == this) {
      continue;
    }
    Point diff = position_ - other->position_;
    double length = diff.length();
    if (0 < length && length < kKinematicSeparation) {
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

void Sprite::Move(const TileMap& map, Point* move) {
  CheckSquares(map, position_, move);
  SetPosition(position_ + *move);
}

void Sprite::SetPosition(const Point& position) {
  position_ = position;
  drawing_position_.x  = divround(position_.x, kTicksPerPixel);
  drawing_position_.y  = divround(position_.y, kTicksPerPixel);
  square_.x  = divround(position_.x, kGridTicks);
  square_.y  = divround(position_.y, kGridTicks);
}

}  // namespace skishore
