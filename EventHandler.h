#ifndef __SKISHORE_EVENT_HANDLER_H__
#define __SKISHORE_EVENT_HANDLER_H__

#include <set>
#include <SDL2/SDL.h>

namespace skishore {

class EventHandler {
 public:
  EventHandler(int events_per_frame);

  void HandleEvents();
  bool IsKeyPressed(const SDL_Keycode& key) const;
  bool IsExitSignaled() const;

 private:
  const int events_per_frame_;
  std::set<SDL_Keycode> keys_pressed_;
  bool exit_signaled_;
};

} // namespace skishore

#endif  // __SKISHORE_EVENT_HANDLER_H__
