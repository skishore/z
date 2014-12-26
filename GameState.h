// GameState is basically a struct, but it also handles memory management
// for sprites and has convenience methods for actually moving them.
//
// GameState's MoveSprite does not do collision checks, because the actual
// game engine should enforce this logic. However, it does throw an assertion
// error if the sprite moves onto another sprite's square, because this
// would violate the data structure's integrity.

#ifndef BABEL_GAME_STATE_H__
#define BABEL_GAME_STATE_H__

#include <unordered_map>
#include <memory>
#include <string>

#include "Point.h"
#include "TileMap.h"

namespace babel {

class FieldOfVision;
class Sprite;

class GameState {
 public:
  GameState(const std::string& map_file);
  ~GameState();

  // AddNPC takes ownership of the new sprite.
  void AddNPC(Sprite* sprite);
  void MoveSprite(Sprite* sprite, const Point& move);

  // Returns true if the square is occupied by a sprite.
  bool IsSquareOccupied(const Point& square) const;

  TileMap map;
  Sprite* player;
  std::unordered_map<Point,Sprite*> sprites;
  std::unique_ptr<FieldOfVision> player_vision;

 private:
  void RecomputePlayerVision();
};

}  // namespace babel

#endif  // BABEL_GAME_STATE_H__
