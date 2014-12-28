// GameState is basically a struct, but it also handles memory management
// for sprites and has convenience methods for actually moving them.
//
// GameState's MoveSprite does not do collision checks, because the actual
// game engine should enforce this logic. However, it does throw an assertion
// error if the sprite moves onto another sprite's square, because this
// would violate the data structure's integrity.

#ifndef BABEL_GAME_STATE_H__
#define BABEL_GAME_STATE_H__

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "FieldOfVision.h"
#include "Point.h"
#include "TileMap.h"

namespace babel {

class Sprite;

class GameState {
 public:
  GameState(const std::string& map_file);
  ~GameState();

  // AddNPC takes ownership of the new sprite.
  void AddNPC(Sprite* sprite);
  void RemoveNPC(Sprite* sprite);
  void MoveSprite(Sprite* sprite, const Point& move);

  void AddLogLine(const std::string& line);
  void CoalesceLog();

  bool IsSquareOccupied(const Point& square) const;
  bool IsSquareSeen(const Point& square) const;
  Sprite* SpriteAt(const Point& square) const;

  TileMap map;
  Sprite* player;
  std::vector<Sprite*> sprites;
  std::unique_ptr<FieldOfVision> player_vision;
  std::deque<std::string> log;

 private:
  void RecomputePlayerVision();
  std::vector<std::vector<bool>> seen;
  std::unordered_map<Point,Sprite*> sprite_positions;
  // The index up to which the log has been coalesced.
  int log_index = 0;
};

}  // namespace babel

#endif  // BABEL_GAME_STATE_H__
