#include <algorithm>

#include "constants.h"
#include "debug.h"
#include "GameState.h"
#include "SpriteState.h"

using std::max;
using std::vector;

namespace skishore {

namespace {

const int kWalkingAnimationFrames = 6;

// Returns a free direction to move to from the given square.
Direction GetFreeDirection(const Point& square, const TileMap& map) {
  int dir;
  for (int i = 0; i < 4; i++) {
    dir = rand() % 4;
    if (map.CheckSquare(square + kShift[dir])) {
      break;
    }
  }
  return (Direction)dir;
}

SpriteState* MoveSprite(const vector<Sprite*>& sprites, const TileMap& map,
                        Sprite* sprite, Point* move, int* anim_num) {
  sprite->AvoidOthers(sprites, move);
  sprite->CheckSquares(move);
  if (!move->zero()) {
    sprite->SetPosition(sprite->GetPosition() + *move);
    int animation_frames = kWalkingAnimationFrames*kPlayerSpeed/move->length();
    if (*anim_num % (2*animation_frames) >= animation_frames) {
      sprite->frame_.x += 4;
    }
    *anim_num = (*anim_num + 1) % (2*animation_frames);
  }
  return nullptr;
}

}  // namespace

void SpriteState::Register(Sprite* sprite) {
  ASSERT(sprite != nullptr, "Registering NULL sprite!");
  ASSERT(sprite_ == nullptr, "Re-registering sprite!");
  sprite_ = sprite;
}

SpriteState* PausedState::MaybeTransition(const GameState& game_state) const {
  if (sprite_->is_player_) {
    return new WalkingState;
  }
  if (steps_ > 0) {
    return nullptr;
  }
  Direction dir = GetFreeDirection(sprite_->GetSquare(), game_state.map_);
  int base_steps = kGridTicks/kEnemySpeed;
  return new RandomWalkState(dir, rand() % (4*base_steps));
}

SpriteState* PausedState::Update(const GameState& game_state) {
  sprite_->frame_.x = sprite_->dir_;
  steps_ -= 1;
  return nullptr;
}

SpriteState* RandomWalkState::MaybeTransition(
    const GameState& game_state) const {
  if (steps_ > 0) {
    return nullptr;
  }
  int base_steps = kGridTicks/kEnemySpeed;
  return new PausedState((rand() % (2*base_steps)) - base_steps);
}

SpriteState* RandomWalkState::Update(const GameState& game_state) {
  sprite_->dir_ = dir_;
  sprite_->frame_.x = sprite_->dir_;
  steps_ -= 1;

  Point move = kEnemySpeed*kShift[dir_];
  MoveSprite(game_state.sprites_, game_state.map_, sprite_, &move, &anim_num_);
  return nullptr;
}

SpriteState* WalkingState::MaybeTransition(const GameState& game_state) const {
  return nullptr;
}

SpriteState* WalkingState::Update(const GameState& game_state) {
  Direction last_dir = sprite_->dir_;
  Point move;

  for (int dir = 0; dir < 4; dir++) {
    if (game_state.input_.IsKeyPressed(kDirectionKey[dir])) {
      last_dir = (Direction)dir;
      move += kShift[dir];
    }
  }
  if (last_dir != sprite_->dir_ &&
      !game_state.input_.IsKeyPressed(kDirectionKey[sprite_->dir_])) {
    sprite_->dir_ = last_dir;
  }
  sprite_->frame_.x = sprite_->dir_;

  move.set_length(kPlayerSpeed);
  MoveSprite(game_state.sprites_, game_state.map_, sprite_, &move, &anim_num_);
  return nullptr;
}

} // namespace skishore
