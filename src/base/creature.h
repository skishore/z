#ifndef __BABEL_BASE_CREATURE_H__
#define __BABEL_BASE_CREATURE_H__

#include <string>
#include <vector>

namespace babel {

struct Creature {
  struct Appearance {
    std::string name;
    int graphic;
  } appearance;

  struct Attack {
    int dice;
    int sides;
  } attack;

  struct Stats {
    int max_health;
    int speed;
    int vision_radius;
  } stats;
};

extern const int mPlayer;
extern const int mGecko;
extern const int mDrone;

extern const std::vector<Creature> kCreatures;

}  // namespace babel

#endif  // __BABEL_BASE_CREATURE_H__
