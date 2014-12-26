#include "creature.h"
#include "debug.h"
#include "FieldOfVision.h"
#include "GameState.h"
#include "Sprite.h"

namespace babel {

GameState::GameState(const string& map_file) {
  map.LoadMap(map_file);
  player = new Sprite(map.GetStartingSquare(), PLAYER);
  AddNPC(player);
  RecomputePlayerVision();

  for (int i = 1; i < map.GetRooms().size(); i++) {
    const TileMap::Room& room = map.GetRooms()[i];
    const int num_enemies = (room.size.x + room.size.y)/2 - 3;
    for (int j = 0; j < num_enemies; j++) {
      Point square = room.position;
      square.x += rand() % room.size.x;
      square.y += rand() % room.size.y;
      CreatureType type = (CreatureType)((rand() % (kNumCreatures - 1)) + 1);
      AddNPC(new Sprite(square, type));
    }
  }
}

GameState::~GameState() {
  for (auto pair : sprites) {
    delete pair.second;
  }
}

void GameState::AddNPC(Sprite* sprite) {
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(!IsSquareOccupied(sprite->square),
         "Adding sprite at occupied square " << sprite->square);
  sprites[sprite->square] = sprite;
}

void GameState::MoveSprite(Sprite* sprite, const Point& move) {
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(sprites[sprite->square] == sprite,
         "Integrity error: unexpected sprite at " << sprite->square);
  Point new_square = sprite->square + move;
  ASSERT(!IsSquareOccupied(new_square),
         "Moving sprite to occupied square " << new_square);
  sprites.erase(sprite->square);
  sprites[new_square] = sprite;
  sprite->square = new_square;

  if (sprite == player) {
    RecomputePlayerVision();
  }
}

bool GameState::IsSquareOccupied(const Point& square) const {
  return sprites.find(square) != sprites.end();
}

void GameState::RecomputePlayerVision() {
  player_vision.reset(
      new FieldOfVision(map, player->square,
                        player->creature->stats.vision_radius));
}

}  // namespace babel
