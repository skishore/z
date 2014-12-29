#ifndef BABEL_ENGINE_H__
#define BABEL_ENGINE_H__

#include "GameState.h"
#include "Point.h"
#include "Sprite.h"

namespace babel {

class Engine {
 public:
  Engine();

  const GameState& GetGameState() const { return game_state_; }

  // Returns true if the command was valid.
  // If this method returns false, the view does not need to be redrawn.
  bool Update(bool has_input, char ch);

 private:
  GameState game_state_;
};

}  // namespace babel

#endif  // BABEL_ENGINE_H__
