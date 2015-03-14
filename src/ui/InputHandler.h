#ifndef __BABEL_UI_INPUT_HANDLER_H__
#define __BABEL_UI_INPUT_HANDLER_H__

#include "engine/Action.h"

namespace babel {
namespace ui {

struct InputResult {
  bool done = false;
  engine::Action* action = nullptr;
};

class InputHandler {
 public:
  InputResult GetInput() const;
};

}  // namespace ui 
}  // namespace babel

#endif  // __BABEL_UI_INPUT_HANDLER_H__
