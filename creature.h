#ifndef BABEL_CREATURE_H__
#define BABEL_CREATURE_H__

#include <string>

using std::string;

namespace babel {

static const int kNumCreatures = 3;

enum CreatureType {
  PLAYER = 0,
  GRID_BUG = 1,
  KOBOLD = 2
};

struct Creature {
  struct Appearance {
    string name;
    char symbol;
    uint32_t color;
  } appearance;

  struct Stats {
    int vision_radius;
  } stats;
};

#define Appearance
#define Stats

static const Creature kCreature[] = {
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

#endif  // BABEL_CREATURE_H__
