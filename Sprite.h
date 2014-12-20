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

namespace battle {
struct BattleData;
}  // namespace battle

class SpriteState;

struct ItemData {
  enum ItemStatus {
    HIDDEN = 0,
    FOREGROUND = 1,
    BACKGROUND = 2
  };
  ItemStatus status = HIDDEN;
  Point frame;
  Point offset;
};

class Sprite {
 public:
  Sprite(bool is_player, const Point& square, const Image& Image,
         const TileMap& map, const TileMap::Room* room);
  ~Sprite();

  // Instance methods used by graphics classes to draw the sprite.
  const Point& GetDrawingPosition() const { return drawing_position_; };
  void Draw(const Point& camera, const SDL_Rect& bounds,
            SDL_Surface* surface) const;

  // Instance methods used by SpriteStates to implement their logic.
  const Point& GetPosition() const { return position_; };
  const TileMap::Room* GetRoom() const { return room_; };
  const Point& GetSquare() const { return square_; };
  SpriteState* GetState() const;

  // Instance methods used as utilities when computing the sprite's move.
  bool CheckSquare(const Point& square) const;
  void CheckSquares(Point* move) const;
  bool HasLineOfSight(const Sprite& other) const;

  void SetPosition(const Point& position);
  void SetState(SpriteState* state);

  // Members exposed so that SpriteState subclasses can use them.
  const bool is_player_;
  Point frame_;
  Direction dir_;
  ItemData item_;
  std::unique_ptr<SpriteState> state_;
  std::unique_ptr<battle::BattleData> battle_;

 private:
  const Image& image_;
  const TileMap& map_;
  const TileMap::Room* room_;

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
