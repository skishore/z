#ifndef BABEL_ENGINE_H__
#define BABEL_ENGINE_H__

#include <vector>

#include "base/point.h"
#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class Engine {
 public:
  Engine();

  void AddEventHandler(EventHandler* handler);

  // Runs a single update step. The player's input action may be null.
  // If the input is not null, this method will take ownership of it.
  bool Update(Action* input);

  const GameState& GetGameState() const { return game_state_; }

 private:
  GameState game_state_;
  std::vector<EventHandler*> handlers_;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_ENGINE_H__
