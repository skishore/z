#ifndef __SKISHORE_GAME_STATE_H__
#define __SKISHORE_GAME_STATE_H__

#include <map>
#include <memory>
#include <vector>

#include "Battle.h"
#include "ImageCache.h"
#include "InputHandler.h"
#include "Point.h"
#include "Sprite.h"
#include "SpriteState.h"
#include "TileMap.h"

namespace skishore {

class GameState {
 public:
  GameState(const InputHandler& input, const TileMap& map, ImageCache* cache);

  // GetSprites is not const because it sorts the sprites in draw order.
  const Point& GetCenter() const { return center_; }
  const std::vector<Sprite*>& GetSprites();

  // Runs all the sprites through a single time step.
  void Update();

  // Members exposed so that SpriteState subclasses can read them.
  const InputHandler& input_;
  const TileMap& map_;
  Sprite* player_;
  std::vector<Sprite*> sprites_;

 private:
  void ComputeCenter(bool snap=false);
  void CreateSprite(
      const Point& starting_square, bool is_player, const TileMap::Room* room);

  ImageCache* cache_;
  std::unique_ptr<battle::Battle> battle_;
  std::map<Sprite*, std::unique_ptr<Sprite>> sprite_ownership_;

  // The point on which the camera should be centered.
  // The last center is tracked for smooth camera movement.
  Point center_;
  Point last_center_;
};

} // namespace skishore

#endif  // __SKISHORE_GAME_STATE_H__
