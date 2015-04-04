// GameState is basically a struct, but it also handles memory management
// for sprites and has convenience methods for actually moving them.
//
// GameState's MoveSprite does not do collision checks, because the actual
// game engine should enforce this logic. However, it does throw an assertion
// error if the sprite moves onto another sprite's square, because this
// would violate the data structure's integrity.

#ifndef __BABEL_ENGINE_GAME_STATE_H__
#define __BABEL_ENGINE_GAME_STATE_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/point.h"
#include "engine/FieldOfVision.h"
#include "engine/Log.h"
#include "engine/TileMap.h"

namespace babel {

namespace dialog {
class Dialog;
}  // namespace dialog

namespace engine {

class Sprite;

class GameState {
 public:
  GameState(const std::string& map_file);
  ~GameState();

  // AddNPC takes ownership of the new sprite.
  void AddNPC(Sprite* sprite);
  void RemoveNPC(Sprite* sprite);
  void MoveSprite(const Point& move, Sprite* sprite);

  Sprite* GetCurrentSprite() const;
  void AdvanceSprite();

  bool IsSquareOccupied(const Point& square) const;
  bool IsSquareSeen(const Point& square) const;
  Sprite* SpriteAt(const Point& square) const;

  void RecomputePlayerVision();

  Sprite* player;
  std::vector<Sprite*> sprites;
  std::unique_ptr<TileMap> map;
  std::unique_ptr<FieldOfVision> player_vision;
  std::unique_ptr<dialog::Dialog> dialog;
  Log log;

 private:
  std::vector<std::vector<bool>> seen;
  std::unordered_map<Point,Sprite*> sprite_positions;
  int sprite_index = 0;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_GAME_STATE_H__
