#include "debug.h"
#include "Sprite.h"
#include "SpriteState.h"

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
  if (state == nullptr) {
    return;
  }
  // TODO(skishore): If we add enter/exit methods to states, call them here.
  state_.reset(state);
  state_->Register(this);
}

}  // namespace skishore
