#include "constants.h"
#include "Sprite.h"
#include "SpriteState.h"

namespace skishore {

Sprite::Sprite(bool is_player, const Point& square, const Image& image)
    : is_player_(is_player), image_(image), position_(kGridSize*square) {}

Point Sprite::GetPosition() const {
  return Point(position_);
}

void Sprite::Draw(const Point& camera, const SDL_Rect& bounds,
                  SDL_Surface* surface) const {
  image_.Draw(Point(position_) - camera, frame_, bounds, surface);
}

}  // namespace skishore
