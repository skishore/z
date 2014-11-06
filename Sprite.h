#ifndef __SKISHORE_SPRITE_H__
#define __SKISHORE_SPRITE_H__

#include <string>
#include <SDL2/SDL.h>

#include "ImageCache.h"
#include "Point.h"

namespace skishore {

class Sprite {
 public:
  Sprite(const Point& size, ImageCache* cache_);
  ~Sprite();
  
  bool LoadImage(const std::string& filename);

 private:
  Point size_;
  Point position_;
  Point frame_;

  // The sprite's actual image data is owned by the ImageCache.
  ImageCache* cache_;
  SDL_Surface* image_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
