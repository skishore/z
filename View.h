#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include <vector>

#include "constants.h"
#include "GameState.h"

namespace babel {

struct TileView {
  char symbol;
  uint32_t color;
};

class View {
 public:
  View(int radius, const GameState& game_state);

  int size;
  std::vector<std::vector<TileView>> tiles;
};

}  // namespace babel

#endif  // BABEL_VIEW_H__
