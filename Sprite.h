#ifndef BABEL_SPRITE_H__
#define BABEL_SPRITE_H__

#include <string>
#include <vector>

#include "creature.h"
#include "GameState.h"
#include "Point.h"

namespace babel {

class Action;

class Sprite {
 public:
  Sprite(const Point& square, int type);

  // Runs the sprite's AI logic and returns an action to take.
  // If this sprite is the player, it may consume the input character ch,
  // in which case it will set has_input to false.
  Action* GetAction(const GameState& game_state, char ch, bool* has_input);

  bool IsAlive() const;
  bool IsPlayer() const;

  Point square;
  const Creature& creature;
  std::string text;
  int cur_health;
  int max_health;

 private:
  Action* GetNPCAction(const GameState& game_state);
  Action* GetPlayerAction(const GameState& game_state, char ch);

  int type;
};

}  // namespace babel

#endif  // BABEL_SPRITE_H__
