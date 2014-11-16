#include <algorithm>

#include "Image.h"
#include "ImageCache.h"

using std::max;
using std::min;
using std::string;

namespace skishore {

Image::Image(const Point& size, SDL_Surface* surface, ImageCache* cache)
    : size_(size), surface_(surface), cache_(cache) {}

Image::~Image() {
  cache_->FreeImage(this);
}

void Image::Draw(const Point& position, const Point& frame,
                 const SDL_Rect& bounds, SDL_Surface* surface) {
  SDL_Rect source;
  SDL_Rect target;
  if (!PositionRects(position, bounds, &source, &target)) {
    return;
  }
  source.x += frame.x*size_.x;
  source.y += frame.y*size_.y;
  SDL_BlitSurface(surface_, &source, surface, &target);
}

bool Image::PositionRects(const Point& position, const SDL_Rect& bounds,
                          SDL_Rect* source, SDL_Rect* target) {
  const int& x = position.x;
  const int& y = position.y;

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
