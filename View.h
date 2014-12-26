#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include <string>
#include <vector>

#include "constants.h"
#include "GameState.h"

namespace babel {

struct TileView {
  struct SpriteView {
    char symbol;
    uint32_t color;
    std::string text;
  };

  char symbol = ' ';
  uint32_t color = 0x00000000;
  SpriteView* sprite;
};

class View {
 public:
  View(int radius, const GameState& game_state);
  ~View();

  int size;
  std::vector<std::vector<TileView>> tiles;
};

}  // namespace babel

#endif  // BABEL_VIEW_H__
