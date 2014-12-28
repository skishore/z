#include "creature.h"

namespace babel {

#define Appearance
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", 0, 0x0060ff60},
    Stats{8}},
  Creature{
    Appearance{"grid bug", 1, 0x00ff6060},
    Stats{4}},
  Creature{
    Appearance{"troll", 2, 0x006060ff},
    Stats{4}}
};

#undef Stats
#undef Appearance

}  // namespace babel
