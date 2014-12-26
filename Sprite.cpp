#include "Sprite.h"

using std::string;

namespace babel {

Sprite::Sprite(const Point& s, int type)
    : square(s), creature(kCreatures[type]) {
  if (type != kPlayerType) {
    text = string{(char)('A' + (rand() % 26))};
    text = "kaa";
  }
}

Point Sprite::GetMove(const GameState& game_state) {
  return Point((rand() % 3) - 1, (rand() % 3) - 1);
}

}  // namespace babel
