#ifndef __BABEL_ENGINE_EVENT_HANDLER_H__
#define __BABEL_ENGINE_EVENT_HANDLER_H__

#include <vector>

#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  virtual void BeforeAttack(sid source, sid target) = 0;
  virtual void LaunchDialog(const Point& source) = 0;
};

class DelegatingEventHandler : public EventHandler {
 public:
  void BeforeAttack(sid source, sid target) override {
    for (auto* handler : handlers_) handler->BeforeAttack(source, target);
  }

  void LaunchDialog(const Point& source) override {
    for (auto* handler : handlers_) handler->LaunchDialog(source);
  }

  std::vector<EventHandler*> handlers_;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_EVENT_HANDLER_H__
