#include <algorithm>

#include "Image.h"
#include "ImageCache.h"

using std::max;
using std::min;
using std::string;

namespace skishore {

Image::Image(const string& filename, const Point& size,
             SDL_Surface* surface, ImageCache* cache)
    : filename_(filename), size_(size), surface_(surface), cache_(cache) {}

Image::~Image() {
  cache_->FreeImage(this);
}

void Image::Draw(const Point& position, const Point& frame,
                 const SDL_Rect& bounds, SDL_Surface* surface) const {
  SDL_Rect source;
  SDL_Rect target;
  if (!PositionRects(position, bounds, &source, &target)) {
    return;
  }
  source.x += frame.x*size_.x;
  source.y += frame.y*size_.y;

  if (filename_ == "player.bmp") {
    const int new_frame = 2*(frame.x % 4) + (frame.x >= 4 ? 1 : 0);
    source.x += (new_frame - frame.x)*size_.x;
  }
  source.x /= 2;
  source.y /= 2;
  source.w /= 2;
  source.h /= 2;

  SDL_BlitScaled(surface_, &source, surface, &target);
}

bool Image::PositionRects(const Point& position, const SDL_Rect& bounds,
                          SDL_Rect* source, SDL_Rect* target) const {
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
