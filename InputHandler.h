#ifndef __SKISHORE_INPUT_HANDLER_H__
#define __SKISHORE_INPUT_HANDLER_H__

#include <set>
#include <SDL2/SDL.h>

namespace skishore {

class InputHandler {
 public:
  InputHandler();

  void Poll(int max_num_events);
  bool IsKeyPressed(const SDL_Keycode& key) const;
  bool IsExitSignaled() const;

 private:
  std::set<SDL_Keycode> keys_pressed_;
  bool exit_signaled_;
};

} // namespace skishore

#endif  // __SKISHORE_INPUT_HANDLER_H__
