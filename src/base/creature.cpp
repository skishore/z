#include "creature.h"

namespace babel {

#define Appearance
#define Attack
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", 0},
    Attack{1, 6},
    Stats{12, 60, 7}},
  Creature{
    Appearance{"many-tongues", 1},
    Attack{1, 2},
    Stats{4, 60, 7}},
  Creature{
    Appearance{"demon's head", 2},
    Attack{1, 1},
    Stats{1, 20, 7}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
