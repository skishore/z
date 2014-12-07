#ifndef __SKISHORE_BATTLE_DATA_H__
#define __SKISHORE_BATTLE_DATA_H__

#include <string>

#include "constants.h"

namespace skishore {
namespace battle {

struct BattleData {
  std::string text;
  // Whether the sprite is left or right of the player.
  Direction side;
  // The direction of the sprite's text box. This direction will either
  // equality the sprite's side, or it will be Direction::UP.
  Direction dir;
};

}  // namespace battle
} // namespace skishore

#endif  // __SKISHORE_BATTLE_DATA_H__
