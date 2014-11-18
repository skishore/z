#include <string>

#include "constants.h"
#include "debug.h"
#include "GameState.h"
#include "Image.h"

using std::string;
using std::vector;

namespace skishore {

GameState::GameState(const Point& starting_square, const InputHandler& input,
                     const TileMap& map, ImageCache* cache)
    : input_(input), map_(map), cache_(cache) {
  ASSERT(cache != nullptr, "Initialized with a NULL ImageCache!");
  CreateSprite(starting_square, true);
}

const vector<Sprite*>& GameState::GetSprites() {
  return sprites_;
}

void GameState::Update() {
  for (Sprite* sprite : sprites_) {
    SpriteState* new_state = sprite->GetState()->MaybeTransition(*this);
    if (new_state != nullptr) {
      sprite->SetState(new_state);
    }
  }
  for (Sprite* sprite : sprites_) {
    Position* move = sprite->GetState()->Move(*this);
    if (move != nullptr) {
      delete move;
    }
  }
}

void GameState::CreateSprite(const Point& square, bool is_player) {
  Sprite* sprite = nullptr;
  const Point size(kGridSize, kGridSize);

  if (is_player) {
    const Image* image = cache_->LoadImage(size, "player.bmp");
    sprite = new Sprite(is_player, square, *image, new WalkingState);
    player_ = sprite;
  } else {
    ASSERT(false, "NPCs have not been implemented!");
  }

  ASSERT(sprite != nullptr, "CreateSprite failed! is_player = " << is_player);
  sprites_.push_back(sprite);
  sprite_ownership_[sprite] = std::unique_ptr<Sprite>(sprite);
}

} // namespace skishore
