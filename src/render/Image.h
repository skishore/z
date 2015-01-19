#ifndef __BABEL_IMAGE_H__
#define __BABEL_IMAGE_H__

#include <string>
#include <SDL2/SDL.h>

#include "base/point.h"

namespace babel {
namespace render {

class Image {
 public:
  Image(const Point& size, const std::string& filename);
  Image(const Image& image, Uint32 tint);
  ~Image();

  void Draw(const Point& position, const Point& frame,
            const SDL_Rect& bounds, SDL_Surface* surface) const;

 private:
  bool PositionRects(const Point& position, const SDL_Rect& bounds,
                     SDL_Rect* source, SDL_Rect* target) const;

  // The Image's surface is owned by the cache that constructed it.
  const Point size_;
  mutable SDL_Surface* surface_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_IMAGE_H__
