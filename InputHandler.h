#ifndef __SKISHORE_INPUT_HANDLER_H__
#define __SKISHORE_INPUT_HANDLER_H__

#include <SDL2/SDL.h>

#include "debug.h"

namespace skishore {

class InputHandler {
 public:
  // Returns true and sets ch on success. If this method returns false,
  // the user requested to exit the game.
  bool GetChar(char* c);
};

} // namespace skishore

#endif  // __SKISHORE_INPUT_HANDLER_H__
