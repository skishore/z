#ifndef __BABEL_UI_INPUT_HANDLER_H__
#define __BABEL_UI_INPUT_HANDLER_H__

namespace babel {
namespace ui {

class InputHandler {
 public:
  // Returns true and sets ch on success. If this method returns false,
  // the user requested to exit the game.
  bool GetChar(char* ch);
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_UI_INPUT_HANDLER_H__
