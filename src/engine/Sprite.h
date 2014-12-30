#ifndef BABEL_SPRITE_H__
#define BABEL_SPRITE_H__

#include <string>
#include <vector>

#include "base/creature.h"
#include "base/point.h"
#include "engine/GameState.h"

namespace babel {
namespace engine {

class Action;

class Sprite {
 public:
  Sprite(const Point& square, int type);

  // Methods needed by the game loop to run sprites at the correct speeds.
  bool HasEnergyNeededToMove() const;
  bool GainEnergy();
  void ConsumeEnergy();

  // Runs an NPC's AI logic and returns an action to take.
  // This method will crash if called on the player.
  Action* GetAction(const GameState& game_state) const;

  bool IsAlive() const;
  bool IsPlayer() const;

  Point square;
  const Creature& creature;
  std::string text;
  int cur_health;
  int max_health;

 private:
  int type;
  int energy;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_SPRITE_H__
