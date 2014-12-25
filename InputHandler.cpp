#include "InputHandler.h"

namespace skishore {

bool InputHandler::GetChar(char* c) {
  SDL_Event event;
  while (true) {
    ASSERT(SDL_WaitEvent(&event), SDL_GetError());
    if (event.type == SDL_QUIT) {
      return false;
    } else if (event.type == SDL_KEYDOWN) {
      const SDL_Keycode& key = event.key.keysym.sym;
      if (SDLK_a <= key && key <= SDLK_z) {
        *c = (char)((int)(key - SDLK_a) + (int)'a');
        return true;
      } else if (key == SDLK_ESCAPE) {
        *c = 0x1B;
        return true;
      }
    }
  }
  ASSERT(false, "Reached end of infinite input loop!");
  return false;
}

}  // namespace skishore
