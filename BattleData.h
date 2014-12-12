#ifndef __SKISHORE_BATTLE_DATA_H__
#define __SKISHORE_BATTLE_DATA_H__

#include <string>

#include "constants.h"

namespace skishore {
namespace battle {

struct BattleData {
  // Whether the sprite is left or right of the player.
  Direction side;

  // Fields used to control the sprite's text box.
  Direction dir;
  std::string text;
};

}  // namespace battle
} // namespace skishore

#endif  // __SKISHORE_BATTLE_DATA_H__
