#ifndef __BABEL_ENGINE_SPRITE_H__
#define __BABEL_ENGINE_SPRITE_H__

#include <string>
#include <vector>

#include "base/creature.h"
#include "base/point.h"
#include "engine/FieldOfVision.h"
#include "engine/GameState.h"

namespace babel {
namespace engine {

class Action;
typedef uint32_t sid;

class Sprite {
 public:
  Sprite(const Point& square, int type, bool hostile);

  // Methods needed by the game loop to run sprites at the correct speeds.
  bool HasEnergyNeededToMove() const;
  bool GainEnergy();
  void ConsumeEnergy();

  // Runs an NPC's AI logic and returns an action to take.
  // This method will crash if called on the player.
  Action* GetAction(const GameState& game_state) const;

  sid Id() const { return id; };
  bool IsAlive() const;
  bool IsPlayer() const;

  Point square;
  const Creature& creature;
  int cur_health;
  int max_health;
  bool hostile;
  std::unique_ptr<FieldOfVision> vision;

 private:
  sid id;
  int type;
  int energy;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_SPRITE_H__
