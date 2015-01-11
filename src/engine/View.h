#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include <map>
#include <string>
#include <vector>

#include "engine/GameState.h"
#include "engine/Sprite.h"

namespace babel {
namespace engine {

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

struct StatusView {
  int cur_health;
  int max_health;
};

class View {
 public:
  View(int radius, const GameState& game_state);

  int size;
  Point offset;
  std::vector<std::vector<TileView>> tiles;
  std::map<sid,SpriteView> sprites;
  std::vector<std::string> log;
  StatusView status;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_VIEW_H__