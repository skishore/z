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

  // Runs the sprite's AI logic and returns an action to take.
  // If this sprite is the player, it may consume the input character ch,
  // in which case it will set has_input to false.
  Action* GetAction(
      const GameState& game_state, char ch, bool* has_input) const;

  bool IsAlive() const;
  bool IsPlayer() const;

  Point square;
  const Creature& creature;
  std::string text;
  int cur_health;
  int max_health;

 private:
  Action* GetNPCAction(const GameState& game_state) const;
  Action* GetPlayerAction(const GameState& game_state, char ch) const;

  int type;
  int energy;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_SPRITE_H__
