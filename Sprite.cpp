#include "Sprite.h"

namespace babel {

Sprite::Sprite(const Point& s, CreatureType type)
    : square(s), creature(&kCreature[type]) {}

Point Sprite::GetMove(const GameState& game_state) {
  return Point((rand() % 3) - 1, (rand() % 3) - 1);
}

}  // namespace babel
