#include "creature.h"

namespace babel {

#define Appearance
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", '@', 0x0060ff60},
    Stats{15}},
  Creature{
    Appearance{"grid bug", 'X', 0x006060ff},
    Stats{15}},
  Creature{
    Appearance{"kobold", 'K', 0x00ff6060},
    Stats{15}}
};

#undef Stats
#undef Appearance

}  // namespace babel
