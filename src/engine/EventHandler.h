#ifndef __BABEL_ENGINE_EVENT_HANDLER_H__
#define __BABEL_ENGINE_EVENT_HANDLER_H__

#include <vector>

#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  virtual void OnAttack(sid source, sid target) = 0;
  virtual void OnSnapshot() = 0;
};

class DelegatingEventHandler : public EventHandler {
 public:
  void OnAttack(sid source, sid target) override {
    for (auto* handler : handlers_) handler->OnAttack(source, target);
  }

  void OnSnapshot() override {
    for (auto* handler : handlers_) handler->OnSnapshot();
  }

  std::vector<EventHandler*> handlers_;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_EVENT_HANDLER_H__
