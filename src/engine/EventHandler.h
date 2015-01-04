#ifndef BABEL_EVENT_HANDLER_H__
#define BABEL_EVENT_HANDLER_H__

#include "base/point.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  virtual void HandleMove(const Point& source, const Point& target);
  virtual void HandleAttack(const Point& source, const Point& target);
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_EVENT_HANDLER_H__
