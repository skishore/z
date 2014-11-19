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

  // Instance methods used by graphics classes to draw the sprite.
  const Point& GetDrawingPosition() const { return drawing_position_; };
  void Draw(const Point& camera, const SDL_Rect& bounds,
            SDL_Surface* surface) const;

  // Instance methods used by SpriteStates to implement their logic.
  const Point& GetPosition() const { return position_; };
  const Point& GetSquare() const { return square_; };
  SpriteState* GetState() const;

  // Instance methods used by GameState on each call to Update.
  void SetState(SpriteState* state);
  void AvoidOthers(const std::vector<Sprite*> sprites, Point* move) const;
  void Move(const TileMap& map, Point* move);

  // Members exposed so that SpriteState subclasses can read them.
  const bool is_player_;
  Point frame_;
  Direction direction_;
  std::unique_ptr<SpriteState> state_;

 private:
  void SetPosition(const Point& position);

  const Image& image_;

  // WARNING: position_ is stored in ticks, not in pixels, where each tick is
  // kGridResolution pixels. This makes the collision detection math work well,
  // but as a result, position_ should never be exposed except via the API.
  Point position_;

  // The sprite's drawing position is in pixels. Its square is a map square.
  // These two members are entirely a function of the precise position.
  Point drawing_position_;
  Point square_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
