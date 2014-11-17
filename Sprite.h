#ifndef __SKISHORE_SPRITE_H__
#define __SKISHORE_SPRITE_H__

#include <memory>
#include <SDL2/SDL.h>

#include "constants.h"
#include "Image.h"
#include "Point.h"

namespace skishore {

class SpriteState;

class Sprite {
 public:
  Sprite(bool is_player, const Point& square, const Image& Image);

  Point GetPosition() const;
  void Draw(const Point& camera, const SDL_Rect& bounds,
            SDL_Surface* surface) const;

 private:
  friend class SpriteState;

  const bool is_player_;
  const Image& image_;

  Point frame_;
  Position position_;
  Direction direction_;
  std::unique_ptr<SpriteState> state_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
