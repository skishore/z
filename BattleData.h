#ifndef __SKISHORE_BATTLE_DATA_H__
#define __SKISHORE_BATTLE_DATA_H__

#include <string>

#include "constants.h"

namespace skishore {
namespace battle {

struct BattleData {
  std::string text;
  Direction dir;
};

}  // namespace battle
} // namespace skishore

#endif  // __SKISHORE_BATTLE_DATA_H__
