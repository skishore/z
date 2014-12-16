#include <algorithm>
#include <string>

#include "constants.h"
#include "debug.h"
#include "GameState.h"
#include "Image.h"

using std::string;
using std::vector;

namespace skishore {

namespace {

static bool TopToBottom(const Sprite* a, const Sprite* b) {
  return a->GetPosition().y < b->GetPosition().y;
}

// The maximum distance the camera can move in one step.
const static int kCameraSpeed = 2*kPlayerSpeed;

}  // namespace

GameState::GameState(
    const InputHandler& input, const TileMap& map, ImageCache* cache)
    : input_(input), map_(map), cache_(cache) {
  ASSERT(cache != nullptr, "Initialized with a NULL ImageCache!");
  CreateSprite(map.GetStartingSquare(), true, nullptr);

  const int num_rooms = map.GetRooms().size();
  if (num_rooms <= 1) {
    return;
  }
  for (int i = 1; i < map.GetRooms().size() ; i++) {
    const TileMap::Room& room = map.GetRooms()[i];
    const int num_enemies = (room.size.x + room.size.y)/2 - 3;
    for (int j = 0; j < num_enemies; j++) {
      Point square = room.position;
      square.x += rand() % room.size.x;
      square.y += rand() % room.size.y;
      CreateSprite(square, false, &room);
    }
  }
  ComputeCenter(true /* snap */);
}

const vector<Sprite*>& GameState::GetSprites() {
  return sprites_;
}

void GameState::Update() {
  if (battle_ != nullptr && battle_->Update()) {
    battle_.reset(nullptr);
  }
  for (Sprite* sprite : sprites_) {
    sprite->SetState(sprite->GetState()->MaybeTransition(*this));
  }
  for (Sprite* sprite : sprites_) {
    sprite->SetState(sprite->GetState()->Update(*this));
    if (battle_ == nullptr && sprite->GetRoom() != nullptr &&
        !sprite->is_player_ && sprite->HasLineOfSight(*player_)) {
      battle_.reset(new battle::Battle(*this, *sprite));
    }
  }
  ComputeCenter();
  std::sort(sprites_.begin(), sprites_.end(), TopToBottom);
}

void GameState::ComputeCenter(bool snap) {
  last_center_ = center_;
  if (battle_ != nullptr) {
    center_ = battle_->GetCenter();
  } else {
    center_ = player_->GetPosition() + Point(kGridTicks/2, kGridTicks/2);
  }
  Point diff = center_ - last_center_;
  if (!snap && abs(diff.x) + abs(diff.y) > kCameraSpeed) {
    double length = diff.length();
    if (length > kCameraSpeed) {
      diff.set_length(kCameraSpeed);
      center_ = last_center_ + diff;
    }
  }
}

void GameState::CreateSprite(
    const Point& square, bool is_player, const TileMap::Room* room) {
  Sprite* sprite = nullptr;
  const Point size(kGridSize, kGridSize);

  if (is_player) {
    const Image* image = cache_->LoadImage(size, "player.bmp");
    sprite = new Sprite(is_player, square, *image, map_, room);
    player_ = sprite;
  } else {
    const Image* image = cache_->LoadImage(size, "zombie.bmp");
    sprite = new Sprite(is_player, square, *image, map_, room);
  }

  ASSERT(sprite != nullptr, "CreateSprite failed! is_player = " << is_player);
  sprites_.push_back(sprite);
  sprite_ownership_[sprite] = std::unique_ptr<Sprite>(sprite);
}

}  // namespace skishore
