#ifndef BABEL_EVENT_HANDLER_H__
#define BABEL_EVENT_HANDLER_H__

#include <vector>

#include "base/point.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

class EventHandler {
 public:
  virtual void BeforeAttack(const Sprite& sprite, const Sprite& target) {};
  virtual void BeforeSpeech(const Sprite& sprite, float radius,
                            const std::vector<Point>& earshot) {};
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_EVENT_HANDLER_H__
