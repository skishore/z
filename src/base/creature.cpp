#include "creature.h"

namespace babel {

#define Appearance
#define Attack
#define Stats

const int mPlayer = 0;
const int mGecko = 1;
const int mDrone = 2;

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", 0},
    Attack{1, 6},
    Stats{12, 60, 9}},
  Creature{
    Appearance{"gecko", 1},
    Attack{1, 1},
    Stats{2, 40, 9}},
  Creature{
    Appearance{"drone", 2},
    Attack{1, 1},
    Stats{1, 20, 9}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
