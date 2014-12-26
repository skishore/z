#ifndef BABEL_SPRITE_H__
#define BABEL_SPRITE_H__

#include <vector>

#include "creature.h"
#include "GameState.h"
#include "Point.h"

namespace babel {

class Sprite {
 public:
  Sprite(const Point& square, CreatureType type);

  // Returns this sprite's move, if the sprite is an NPC.
  Point GetMove(const GameState& game_state);

  Point square;
  const Creature* creature;
};

}  // namespace babel

#endif  // BABEL_SPRITE_H__
