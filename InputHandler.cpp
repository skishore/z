#include "InputHandler.h"

namespace babel {

bool InputHandler::GetChar(char* ch) {
  SDL_Event event;
  while (true) {
    ASSERT(SDL_WaitEvent(&event), SDL_GetError());
    if (event.type == SDL_QUIT) {
      return false;
    } else if (event.type == SDL_KEYDOWN) {
      const SDL_Keycode& key = event.key.keysym.sym;
      if (SDLK_a <= key && key <= SDLK_z) {
        *ch = (char)((int)(key - SDLK_a) + (int)'a');
        return true;
      } else if (key == SDLK_PERIOD) {
        *ch = '.';
        return true;
      } else if (key == SDLK_ESCAPE) {
        *ch = 0x1B;
        return true;
      }
    }
  }
  ASSERT(false, "Reached end of infinite input loop!");
  return false;
}

}  // namespace babel
