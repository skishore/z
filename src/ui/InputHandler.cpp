#include <map>
#include <SDL2/SDL.h>

#include "ui/InputHandler.h"

using std::map;

namespace babel {
namespace ui {

namespace {
map<SDL_Keycode,char> key_to_char{
  {SDLK_BACKSPACE, '\b'},
  {SDLK_ESCAPE, 0x1b},
  {SDLK_PERIOD, '.'},
  {SDLK_RETURN, '\n'},
  {SDLK_SPACE, ' '}
};
}  // namespace

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
    } else if (key_to_char.find(key) != key_to_char.end()) {
      *ch = key_to_char.at(key);
      return true;
    }
  }
  return false;
}

}  // namespace ui
}  // namespace babel
