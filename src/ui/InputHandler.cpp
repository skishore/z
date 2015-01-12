#include <SDL2/SDL.h>

#include "ui/InputHandler.h"

namespace babel {
namespace ui {

bool InputHandler::GetChar(char* ch) {
  SDL_Event event;
  if (!SDL_PollEvent(&event)) {
    return false;
  }
  if (event.type == SDL_QUIT) {
    return false;
  } else if (event.type == SDL_KEYDOWN) {
    const SDL_Keycode& key = event.key.keysym.sym;
    if (SDLK_a <= key && key <= SDLK_z) {
      const char base = (event.key.keysym.mod & KMOD_SHIFT ? 'A' : 'a');
      *ch = (char)((int)(key - SDLK_a) + (int)base);
      return true;
    } else if (key == SDLK_PERIOD) {
      *ch = '.';
      return true;
    } else if (key == SDLK_ESCAPE) {
      *ch = 0x1B;
      return true;
    }
  }
  return false;
}

}  // namespace ui
}  // namespace babel
