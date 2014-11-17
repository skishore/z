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
  Sprite(bool is_player, const Point& square,
         const Image& Image, SpriteState* state);

  Point GetPosition() const;
  void Draw(const Point& camera, const SDL_Rect& bounds,
            SDL_Surface* surface) const;

  SpriteState* GetState();
  void SetState(SpriteState* state);

  // Members exposed so that SpriteState subclasses can read them.
  const bool is_player_;
  Point frame_;
  Position position_;
  Direction direction_;
  std::unique_ptr<SpriteState> state_;

 private:
  const Image& image_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
