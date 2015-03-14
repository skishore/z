#ifndef EMSCRIPTEN

#include "ui/InputHandler.h"

#include <map>
#include <ncurses.h>

#include "base/point.h"

using std::map;

namespace babel {
namespace ui {
namespace {

// Mapping from directional keys to the desired move.
static const map<char,Point> kCharToMove = {
  {'h', Point(-1, 0)},
  {'j', Point(0, 1)},
  {'k', Point(0, -1)},
  {'l', Point(1, 0)},
  {'y', Point(-1, -1)},
  {'u', Point(1, -1)},
  {'b', Point(-1, 1)},
  {'n', Point(1, 1)},
  {'.', Point(0, 0)}
};

}  // namespace

InputResult InputHandler::GetInput() const {
  InputResult result;
  const char ch = getch();
  if (ch == 0x03 /* Ctrl-C */) {
    result.done = true;
  } else if (kCharToMove.find(ch) != kCharToMove.end()) {
    result.action = new engine::MoveAction(kCharToMove.at(ch));
  }
  return result;
}

}  // namespace ui 
}  // namespace babel

#endif  // EMSCRIPTEN
