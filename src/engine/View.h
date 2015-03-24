#ifndef __BABEL_ENGINE_VIEW_H__
#define __BABEL_ENGINE_VIEW_H__

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
  // square is offset within the current view.
  engine::sid id;
  int graphic;
  Point square;
  std::string label;
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
  std::vector<SpriteView> sprites;
  std::vector<std::string> log;
  StatusView status;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_VIEW_H__
