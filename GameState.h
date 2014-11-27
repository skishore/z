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
  const Sprite& GetPlayer() const { return *player_; };
  const std::vector<Sprite*>& GetSprites();

  // Runs all the sprites through a single time step.
  void Update();

  // Members exposed so that SpriteState subclasses can read them.
  const InputHandler& input_;
  const TileMap& map_;
  Sprite* player_;
  std::vector<Sprite*> sprites_;

 private:
  void CreateSprite(
      const Point& starting_square, bool is_player, const TileMap::Room* room);

  ImageCache* cache_;
  std::unique_ptr<Battle> battle_;
  std::map<Sprite*, std::unique_ptr<Sprite>> sprite_ownership_;
};

} // namespace skishore

#endif  // __SKISHORE_GAME_STATE_H__
