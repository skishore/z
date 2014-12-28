#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include <string>
#include <vector>

#include "GameState.h"

namespace babel {

struct TileView {
  // graphic will be -1 if the tile is unknown to the player.
  int graphic;
  bool visible;
};

struct SpriteView {
  // The square is offset within the current view.
  int graphic;
  uint32_t color;
  Point square;
  std::string text;
};

class View {
 public:
  View(int radius, const GameState& game_state);

  int size;
  std::vector<std::vector<TileView>> tiles;
  std::vector<SpriteView> sprites;
};

}  // namespace babel

#endif  // BABEL_VIEW_H__
