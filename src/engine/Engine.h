#ifndef BABEL_ENGINE_H__
#define BABEL_ENGINE_H__

#include "base/point.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class Engine {
 public:
  Engine(EventHandler* handler);

  // Runs a single update step. The player's input action may be null.
  // If it is, this method will block the first time the player gets to take
  // a turn; if not, it  will consume the action and block the second time.
  bool Update(Action* input, bool* used_input);

  const GameState& GetGameState() const { return game_state_; }

 private:
  GameState game_state_;
  EventHandler* handler_;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_ENGINE_H__
