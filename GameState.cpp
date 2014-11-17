#include <string>

#include "constants.h"
#include "GameState.h"
#include "Image.h"

using std::string;
using std::vector;

namespace skishore {

GameState::GameState(const Point& starting_square, const InputHandler& input,
                     const TileMap& map, ImageCache* cache)
    : input_(input), map_(map), cache_(cache) {
  CreateSprite(starting_square, true);
}

const vector<Sprite*>& GameState::GetSprites() {
  return sprites_;
}

void GameState::Update() {
}

void GameState::CreateSprite(const Point& square, bool is_player) {
  const string filename = (is_player ? "player.bmp" : "zombie.bmp");
  const Image* image = cache_->LoadImage(Point(kGridSize, kGridSize), filename);
  Sprite* sprite = new Sprite(is_player, square, *image);

  if (is_player) {
    player_ = sprite;
  }
  sprites_.push_back(sprite);
  sprite_ownership_[sprite] = std::unique_ptr<Sprite>(sprite);
}

} // namespace skishore
