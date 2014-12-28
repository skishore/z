#ifndef BABEL_SPRITE_H__
#define BABEL_SPRITE_H__

#include <string>
#include <vector>

#include "creature.h"
#include "GameState.h"
#include "Point.h"

namespace babel {

class SpriteAPI {
 public:
  virtual void Attack(Sprite* sprite, Sprite* target) = 0;
  virtual void Move(const Point& move, Sprite* sprite) = 0;
};

class Sprite {
 public:
  Sprite(const Point& square, int type);

  // Runs the sprite's update logic and calls out to the API to move.
  void Update(const GameState& game_state, SpriteAPI* api);

  Point square;
  const Creature& creature;
  std::string text;
};

}  // namespace babel

#endif  // BABEL_SPRITE_H__
