#ifndef __BABEL_ENGINE_ENGINE_H__
#define __BABEL_ENGINE_ENGINE_H__

#include <memory>
#include <vector>

#include "base/point.h"
#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"
#include "interface/Dialog.h"

namespace babel {
namespace engine {

class Engine {
 public:
  Engine();

  void AddEventHandler(EventHandler* handler);

  // Returns a non-null dialog if this game is blocking on input.
  interface::Dialog* GetDialog();

  // Runs a single update step. The player's input action may be null.
  // If the input is not null, this method will take ownership of it.
  bool Update(Action* input);

  const GameState& GetGameState() const { return game_state_; }

 private:
  GameState game_state_;
  std::vector<EventHandler*> handlers_;
  std::unique_ptr<Action> interrupt_;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_ENGINE_H__
