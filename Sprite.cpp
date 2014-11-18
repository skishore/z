#include <algorithm>

#include "debug.h"
#include "Sprite.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

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
    double diff_length = diff.length();
    if (kZero < diff_length && diff_length < kKinematicSeparation) {
      double length = kKinematicSensitivity/max(diff_length, kKinematicMinDist);
      diff *= length/diff_length;
      if (is_player_) {
        diff.x = (diff.x*move->x > kZero ? 0 : diff.x);
        diff.y = (diff.y*move->y > kZero ? 0 : diff.y);
      }
      net += diff;
    }
  }

  double backoff = (rand() % 2)*kKinematicBackoff;
  if (is_player_) {
    net *= kKinematicPlayerForce;
    backoff = 0;
  }
  if (net.x*move->x < kZero) {
    move->x = (abs(move->x) < abs(net.x) ? backoff*net.x : move->x + net.x);
  }
  if (net.y*move->y < kZero) {
    move->y = (abs(move->y) < abs(net.y) ? backoff*net.y : move->y + net.y);
  }
}

void Sprite::Move(const TileMap& map, Position* move) {
}

}  // namespace skishore
