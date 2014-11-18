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

static const int kTicksPerPixel = 1024;
static const int kTolerance = 204*kGridSize;
static const int kPushAway =  512*kGridSize;
static const int kFullGrid = kTicksPerPixel*kGridSize;

// Takes an integer and returns it mod kGridSize, in [0, kGridSize).
inline int gmod(int x) {
  int result = x % kFullGrid;
  return result + (result < 0 ? kFullGrid : 0);
}

bool CheckSquare(const TileMap& map, const Point& square) {
  return map.GetMapTile(square) != 4;
}

void CheckSquares(const TileMap& map, const Point& pos,
                  const Position& dmove, Point* move) {
  move->x = kTicksPerPixel*dmove.x;
  move->y = kTicksPerPixel*dmove.y;
  if (move->x == 0 && move->y == 0) {
    return;
  }

  Point overlap(gmod(pos.x), gmod(pos.y));
  overlap.x -= 2*overlap.x/kFullGrid*kFullGrid;
  overlap.y -= 2*overlap.y/kFullGrid*kFullGrid;

  Point square = pos - overlap;
  ASSERT(square.x % kFullGrid == 0 && square.y % kFullGrid == 0,
         "Unexpected: " << pos << " " << overlap << " " << " " << kFullGrid);
  square = square/kFullGrid;

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
        move->y = kFullGrid - kTolerance - gmod(pos.y);
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
    // TODO(skishore): Why is this line necessary?
    collided = offset.y != 0 && !collided && !CheckSquare(map, square + offset);
    offset.y = (overlap.y > 0 ? 1 : -1);
    if (!CheckSquare(map, Point(square.x + offset.x, square.y))) {
      collided = true;
    } else if ((offset.y > 0 || -overlap.y > kTolerance) &&
               !CheckSquare(map, square + offset)) {
      collided = true;
      if (abs(overlap.y) < kPushAway && offset.y*move->y <= 0) {
        // TODO(skishore): Why is this ternary expression necessary?
        move->y = (CheckSquare(map, Point(square.x, square.y + offset.y)) ?
                   -offset.y*speed : 0);
      }
    }
    if (collided) {
      if (offset.x < 0) {
        move->x = kFullGrid - kTolerance - gmod(pos.x);
      } else {
        move->x = kTolerance - gmod(pos.x);
      }
    }
  }
}

} // namespace

Sprite::Sprite(bool is_player, const Point& square,
               const Image& image, SpriteState* state)
    : is_player_(is_player), direction_(Direction::DOWN), image_(image),
      position_(kTicksPerPixel*kGridSize*square) {
  SetState(state);
}

Point Sprite::GetPosition() const {
  // TODO(skishore): Replace this function with exact arithmetic.
  return Point(Position(position_)/kTicksPerPixel);
}

void Sprite::Draw(const Point& camera, const SDL_Rect& bounds,
                  SDL_Surface* surface) const {
  image_.Draw(GetPosition() - camera, frame_, bounds, surface);
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
  Point final_move;
  CheckSquares(map, position_, *move, &final_move);
  position_ += final_move;
}

}  // namespace skishore
