#include "EventHandler.h"

namespace skishore {

EventHandler::EventHandler(int events_per_frame)
    : events_per_frame_(events_per_frame), exit_signaled_(false) {}

void EventHandler::HandleEvents() {
  SDL_Event event;
  for (int i = 0; (i < events_per_frame_) && SDL_PollEvent(&event); i++) {
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

bool EventHandler::IsKeyPressed(const SDL_Keycode& key) const {
  return keys_pressed_.find(key) != keys_pressed_.end();
}

bool EventHandler::IsExitSignaled() const {
  return exit_signaled_;
}

}  // namespace skishore
