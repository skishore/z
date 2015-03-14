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
    Appearance{"bulbasaur", 1, 0x00ff6060},
    Attack{1, 2},
    Stats{6, 40, 7}},
  Creature{
    Appearance{"charmander", 2, 0x006060ff},
    Attack{1, 3},
    Stats{6, 40, 9}},
  Creature{
    Appearance{"squirtle", 3, 0x006060ff},
    Attack{1, 2},
    Stats{8, 60, 7}},
  Creature{
    Appearance{"rattata", 4, 0x006060ff},
    Attack{1, 2},
    Stats{2, 40, 7}},
  Creature{
    Appearance{"pidgey", 5, 0x006060ff},
    Attack{1, 2},
    Stats{4, 80, 7}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
