#include "creature.h"

namespace babel {

#define Appearance
#define Attack
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", 0},
    Attack{1, 6},
    Stats{12, 60, 9}},
  Creature{
    Appearance{"many-tongues", 1},
    Attack{1, 2},
    Stats{4, 60, 9}},
  Creature{
    Appearance{"no-tongue", 2},
    Attack{1, 1},
    Stats{1, 20, 9}},
  Creature{
    Appearance{"demon's head", 3},
    Attack{1, 1},
    Stats{1, 10, 9}},
  Creature{
    Appearance{"gecko", 4},
    Attack{1, 1},
    Stats{2, 40, 9}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
