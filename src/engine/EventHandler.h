#ifndef BABEL_EVENT_HANDLER_H__
#define BABEL_EVENT_HANDLER_H__

#include "base/point.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  // All event notifications occur right before the actual event.
  virtual void HandleAttack(const Sprite& sprite, const Sprite& target) = 0;
  virtual void HandleMove(const Sprite& sprite, const Point& square) = 0;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_EVENT_HANDLER_H__
