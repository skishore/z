#ifndef __SKISHORE_SPRITE_H__
#define __SKISHORE_SPRITE_H__

#include <memory>
#include <vector>
#include <SDL2/SDL.h>

#include "constants.h"
#include "Image.h"
#include "Point.h"
#include "TileMap.h"

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

  // Applies sprite-sprite forces to the position delta, which cannot be NULL.
  void AvoidOthers(const std::vector<Sprite*> sprites, Position* move) const;

  // Actually moves the sprite within the given map.
  void Move(const TileMap& map, Position* move);

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
