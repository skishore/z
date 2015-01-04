#include "base/debug.h"
#include "engine/EventHandler.h"

namespace babel {
namespace engine {

void EventHandler::HandleAttack(const Point& source, const Point& target) {
  DEBUG("Attack from " << source << " to " << target);
}

void EventHandler::HandleMove(const Point& source, const Point& target) {
  DEBUG("Move from " << source << " to " << target);
}

}  // namespace engine
}  // namespace babel
