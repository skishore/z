#ifndef __BABEL_ENGINE_EVENT_HANDLER_H__
#define __BABEL_ENGINE_EVENT_HANDLER_H__

#include <vector>

#include "base/point.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  virtual void BeforeAttack(const Sprite& sprite, const Sprite& target) {};
  virtual void BeforeSpeech(const Sprite& sprite) {};
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_EVENT_HANDLER_H__
