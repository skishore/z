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
#include "engine/Trap.h"

namespace babel {

namespace dialog {
class Dialog;
}  // namespace dialog

namespace engine {

class EventHandler;
class Sprite;

class GameState {
 public:
  GameState(const std::string& map_file);
  ~GameState();

  // AddNPC takes ownership of the new sprite.
  void AddNPC(Sprite* sprite);
  void RemoveNPC(Sprite* sprite);
  void MoveSprite(const Point& move, Sprite* sprite);

  // AddTrap takes ownership of the new trap.
  void AddTrap(Trap* trap);
  void RemoveTrap(Trap* trap);
  void MaybeTriggerTrap(const Point& square, EventHandler* handler);

  Sprite* GetCurrentSprite() const;
  void AdvanceSprite();

  bool IsSquareOccupied(const Point& square) const;
  Sprite* SpriteAt(const Point& square) const;

  bool IsSquareTrapped(const Point& square) const;
  Trap* TrapAt(const Point& square) const;

  bool IsSquareSeen(const Point& square) const;
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
  std::unordered_map<Point,Trap*> trap_positions;
  std::vector<Trap*> traps;
  int sprite_index = 0;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_GAME_STATE_H__
