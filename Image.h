#ifndef __BABEL_IMAGE_H__
#define __BABEL_IMAGE_H__

#include <SDL2/SDL.h>

#include "Point.h"

namespace babel {

class ImageCache;

class Image {
 public:
  ~Image();

  void Draw(const Point& position, const Point& frame,
            const SDL_Rect& bounds, SDL_Surface* surface) const;

 private:
  // ImageCache is a factory class used to construct Image instances.
  friend class ImageCache;

  Image(const Point& size, SDL_Surface* surface, ImageCache* cache);

  bool PositionRects(const Point& position, const SDL_Rect& bounds,
                     SDL_Rect* source, SDL_Rect* target) const;

  // The Image's surface is owned by the cache that constructed it.
  const Point size_;
  mutable SDL_Surface* surface_;
  ImageCache* const cache_;
};

} // namespace babel

#endif  // __BABEL_IMAGE_H__
