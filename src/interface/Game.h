// Abstract class that is implemented by semantic games. Reports the game
// result when the game is complete.
//
#ifndef __BABEL_INTERFACE_GAME_H__
#define __BABEL_INTERFACE_GAME_H__

#include "interface/Dialog.h"

namespace babel {
namespace interface {

struct GameResult {
  int errors = 0;
};

class Game : public Dialog {
 public:
  virtual GameResult GetResult() const = 0;
};

}  // namespace interface
}  // namespace babel

#endif  // __BABEL_INTERFACE_DIALOG_H__
