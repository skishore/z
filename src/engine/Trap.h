#ifndef __BABEL_ENGINE_TRAP_H__
#define __BABEL_ENGINE_TRAP_H__

#include <vector>

#include "base/point.h"

namespace babel {
namespace engine {

class EventHandler;
class GameState;

class Trap {
 public:
  virtual ~Trap() {}

  // A trap is triggered when the player steps on one of its squares.
  // The trap is destroyed after Trigger is called.
  virtual void Trigger(GameState* game_state, EventHandler* handler) = 0;

  const std::vector<Point>& GetSquares() const { return squares_; }

 protected:
  std::vector<Point> squares_;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_TRAP_H__
