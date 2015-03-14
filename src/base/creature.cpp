#include "creature.h"

namespace babel {

#define Appearance
#define Attack
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", 0, 0x0060ff60},
    Attack{1, 6},
    Stats{12, 60, 11}},
  Creature{
    Appearance{"grid bug", 1, 0x00ff6060},
    Attack{1, 1},
    Stats{2, 40, 7}},
  Creature{
    Appearance{"troll", 2, 0x006060ff},
    Attack{1, 2},
    Stats{4, 30, 7}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
