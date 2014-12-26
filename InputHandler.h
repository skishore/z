#ifndef __BABEL_INPUT_HANDLER_H__
#define __BABEL_INPUT_HANDLER_H__

#include <SDL2/SDL.h>

#include "debug.h"

namespace babel {

class InputHandler {
 public:
  // Returns true and sets ch on success. If this method returns false,
  // the user requested to exit the game.
  bool GetChar(char* ch);
};

} // namespace babel

#endif  // __BABEL_INPUT_HANDLER_H__
