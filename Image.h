#ifndef __SKISHORE_IMAGE_H__
#define __SKISHORE_IMAGE_H__

#include <string>
#include <SDL2/SDL.h>

#include "Point.h"

namespace skishore {

class Image {
 public:
  void Draw(const Point& position, const Point& frame,
            const SDL_Rect& bounds, SDL_Surface* surface);

 private:
  Image(const Point& size, SDL_Surface* surface);

  bool PositionRects(const Point& position, const SDL_Rect& bounds,
                     SDL_Rect* source, SDL_Rect* target);

  // class ImageCache is a factory for Images.
  // The Image's surface is owned by the cache that constructed it.
  Point size_;
  SDL_Surface* surface_;

  friend class ImageCache;
};

} // namespace skishore

#endif  // __SKISHORE_IMAGE_H__
