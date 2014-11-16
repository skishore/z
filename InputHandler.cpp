#include "InputHandler.h"

namespace skishore {

InputHandler::InputHandler() : exit_signaled_(false) {}

void InputHandler::Poll(int max_num_events) {
  SDL_Event event;
  for (int i = 0; (i < max_num_events) && SDL_PollEvent(&event); i++) {
    if (event.type == SDL_QUIT) {
      exit_signaled_ = true;
    } else if (event.type == SDL_KEYDOWN) {
      const SDL_Keycode& key = event.key.keysym.sym;
      switch(key) {
        case SDLK_ESCAPE:
          exit_signaled_ = true;
        default:
          keys_pressed_.insert(key);
      }
    } else if (event.type == SDL_KEYUP) {
      const SDL_Keycode& key = event.key.keysym.sym;
      keys_pressed_.erase(key);
    }
  }
}

bool InputHandler::IsKeyPressed(const SDL_Keycode& key) const {
  return keys_pressed_.find(key) != keys_pressed_.end();
}

bool InputHandler::IsExitSignaled() const {
  return exit_signaled_;
}

}  // namespace skishore
