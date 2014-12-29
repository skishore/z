#ifndef BABEL_CREATURE_H__
#define BABEL_CREATURE_H__

#include <string>
#include <vector>

namespace babel {

struct Creature {
  struct Appearance {
    std::string name;
    int graphic;
    uint32_t color;
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

static const int kPlayerType = 0;
extern const std::vector<Creature> kCreatures;

}  // namespace babel

#endif  // BABEL_CREATURE_H__
