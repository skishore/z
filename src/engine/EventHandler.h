#ifndef BABEL_EVENT_HANDLER_H__
#define BABEL_EVENT_HANDLER_H__

#include "base/point.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  // If the target is killed, AfterAttack is called before it is removed.
  virtual void AfterAttack(const Sprite& sprite, const Sprite& target) {};
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_EVENT_HANDLER_H__
