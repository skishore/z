#ifndef __SKISHORE_GAME_STATE_H__
#define __SKISHORE_GAME_STATE_H__

#include <map>
#include <memory>
#include <vector>

#include "ImageCache.h"
#include "InputHandler.h"
#include "Point.h"
#include "Sprite.h"
#include "SpriteState.h"
#include "TileMap.h"

namespace skishore {

class GameState {
 public:
  GameState(const Point& starting_square, const InputHandler& input,
            const TileMap& map, ImageCache* cache);

  // GetSprites is not const because it sorts the sprites in draw order.
  const Sprite& GetPlayer() const { return *player_; };
  const std::vector<Sprite*>& GetSprites();

  // Runs all the sprites through a single time.
  void Update();

 private:
  friend class SpriteState;

  void CreateSprite(const Point& starting_square, bool is_player);

  const InputHandler& input_;
  const TileMap& map_;
  ImageCache* cache_;

  Sprite* player_;
  std::vector<Sprite*> sprites_;

  std::map<Sprite*, std::unique_ptr<Sprite>> sprite_ownership_;
};

} // namespace skishore

#endif  // __SKISHORE_GAME_STATE_H__
