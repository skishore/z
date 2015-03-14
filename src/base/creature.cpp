#include "creature.h"

#ifdef EMSCRIPTEN
#define COLOR_BLACK   0x00000000
#define COLOR_RED     0x00ff0000
#define COLOR_GREEN   0x0000ff00
#define COLOR_BLUE    0x000000ff
#define COLOR_YELLOW  0x00ffff00
#define COLOR_MAGENTA 0x00ff00ff
#define COLOR_CYAN    0x0000ffff
#define COLOR_WHITE   0x00ffffff
#else
#include <ncurses.h>
#endif

namespace babel {

#define Appearance
#define Attack
#define Stats

const std::vector<Creature> kCreatures = {
  Creature{
    Appearance{"human", '@', COLOR_WHITE},
    Attack{1, 1},
    Stats{12, 60, 11}},
  Creature{
    Appearance{"bulbasaur", 'b', COLOR_GREEN},
    Attack{1, 2},
    Stats{6, 40, 7}},
  Creature{
    Appearance{"charmander", 'c', COLOR_RED},
    Attack{1, 3},
    Stats{6, 40, 9}},
  Creature{
    Appearance{"squirtle", 's', COLOR_CYAN},
    Attack{1, 2},
    Stats{8, 60, 7}},
  Creature{
    Appearance{"rattata", 'r', COLOR_WHITE},
    Attack{1, 2},
    Stats{2, 40, 7}},
  Creature{
    Appearance{"pidgey", 'p', COLOR_YELLOW},
    Attack{1, 2},
    Stats{4, 80, 7}}
};

#undef Stats
#undef Attack
#undef Appearance

}  // namespace babel
