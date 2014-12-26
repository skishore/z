#include "creature.h"

namespace babel {

#define Appearance
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", '@', 0x005fff5f},
    Stats{15}},
  Creature{
    Appearance{"grid bug", 'X', 0x005f5fff},
    Stats{15}},
  Creature{
    Appearance{"kobold", 'K', 0x00ff5f5f},
    Stats{15}}
};

#undef Stats
#undef Appearance

}  // namespace babel
