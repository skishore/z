#ifndef __SKISHORE_SPRITE_H__
#define __SKISHORE_SPRITE_H__

#include <SDL2/SDL.h>

#include "Image.h"
#include "Point.h"

namespace skishore {

class Sprite {
 public:
  Sprite(const Point& square, const Image& Image);

  Point GetPosition() const;
  void Draw(const Point& camera, const SDL_Rect& bounds,
            SDL_Surface* surface) const;

 private:
  friend class Engine;

  // The sprite's image should be a kGridSize x kGridSize square.
  const Image& image_;

  Point frame_;
  Position position_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
