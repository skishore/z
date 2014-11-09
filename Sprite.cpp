#include <algorithm>

#include "Sprite.h"

using std::max;
using std::min;
using std::string;

namespace skishore {

Sprite::Sprite(const Point& size, ImageCache* cache) :
    size_(size), cache_(cache), image_(nullptr) {}

Sprite::~Sprite() {
  cache_->FreeImage(image_);
}

bool Sprite::LoadImage(const string& filename) {
  cache_->FreeImage(image_);
  return cache_->LoadImage(filename, &image_);
}

void Sprite::Draw(
    const SDL_Rect& bounds, const Point& camera, SDL_Surface* surface) {
  SDL_Rect source;
  SDL_Rect target;
  if (!PositionRects(bounds, camera, &source, &target)) {
    return;
  }
  source.x += frame_.x*size_.x;
  source.y += frame_.y*size_.y;
  SDL_BlitSurface(image_, &source, surface, &target);
}

bool Sprite::PositionRects(const SDL_Rect& bounds, const Point& camera,
                           SDL_Rect* source, SDL_Rect* target) {
  const int x = position_.x*size_.x - camera.x;
  const int y = position_.y*size_.y - camera.y;

  // Don't draw the sprite if it is out of the target bounds.
  if ((x + size_.x < 0) or (x > bounds.w) or
      (y + size_.y < 0) or (y > bounds.h)) {
    return false;
  }

  // Place the target rect, taking care around the boundary.
  target->x = max(x, 0);
  target->y = max(y, 0);
  target->w = min(x + size_.x, bounds.w) - target->x;
  target->h = min(y + size_.y, bounds.h) - target->y;

  // Place the source rect based on the target's cutoffs.
  source->x = target->x - x;
  source->y = target->y - y;
  source->w = target->w;
  source->h = target->h;

  // Offset the target by the origin of the the bounding box.
  target->x += bounds.x;
  target->y += bounds.y;
  return true;
}

}  // namespace skishore
