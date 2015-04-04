#include "engine/GameState.h"

#include <algorithm>
#include <map>

#include "base/creature.h"
#include "base/debug.h"
#include "base/util.h"
#include "dialog/Dialog.h"
#include "engine/Sprite.h"
#include "gen/RoomAndCorridorMap.h"

using std::map;
using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {
namespace {

const Point kMapSize(64, 64);

}  // namespace

GameState::GameState(const string& map_file) {
  map.reset(new gen::RoomAndCorridorMap(kMapSize));
  seen = vector<vector<bool>>(
      map->GetSize().x, vector<bool>(map->GetSize().y, false));
  player = new Sprite(map->GetStartingSquare(), mPlayer);
  AddNPC(player);
  RecomputePlayerVision();

  for (int i = 0; i < map->GetRooms().size(); i++) {
    const TileMap::Room& room = map->GetRooms()[i];
    if (room.Contains(player->square)) {
      continue;
    }

    // Decide what how many and what type of enemies to spawn in this room.
    bool worker = rand() % 2 == 0;
    const int num_enemies = (worker ? 1 : (room.size.x + room.size.y)/2 - 3);
    const int type = (worker ? mWorker : mGecko);

    for (int j = 0; j < num_enemies; j++) {
      while (true) {
        Point square = room.position;
        square.x += rand() % room.size.x;
        square.y += rand() % room.size.y;
        if (!IsSquareOccupied(square)) {
          AddNPC(new Sprite(square, type));
          break;
        }
      }
    }
  }
}

GameState::~GameState() {
  for (Sprite* sprite : sprites) {
    delete sprite;
  }
}

void GameState::AddNPC(Sprite* sprite) {
  ASSERT(sprite != nullptr);
  ASSERT(!IsSquareOccupied(sprite->square));
  sprites.push_back(sprite);
  sprite_positions[sprite->square] = sprite;
}

void GameState::RemoveNPC(Sprite* sprite) {
  ASSERT(sprite != nullptr);
  ASSERT(!sprite->IsPlayer());
  const auto& it = remove(sprites.begin(), sprites.end(), sprite);
  const int index = it - sprites.begin();
  sprites.erase(it, sprites.end());
  sprite_positions.erase(sprite->square);
  delete sprite;
  // Update the current-sprite index, if necessary.
  if (sprite_index > index) {
    sprite_index -= 1;
  } else if (sprite_index == index) {
    sprite_index = sprite_index % sprites.size();
  }
}

void GameState::MoveSprite(const Point& move, Sprite* sprite) {
  if (move.x == 0 && move.y == 0) {
    return;
  }
  ASSERT(sprite != nullptr);
  ASSERT(sprite_positions[sprite->square] == sprite);
  Point new_square = sprite->square + move;
  ASSERT(!IsSquareOccupied(new_square));
  sprite_positions.erase(sprite->square);
  sprite_positions[new_square] = sprite;
  sprite->square = new_square;

  if (sprite == player) {
    RecomputePlayerVision();
  }
}

Sprite* GameState::GetCurrentSprite() const {
  return sprites[sprite_index];
}

void GameState::AdvanceSprite() {
  ASSERT(sprites.size() > 0);
  sprite_index = (sprite_index + 1) % sprites.size();
}

bool GameState::IsSquareOccupied(const Point& square) const {
  return sprite_positions.find(square) != sprite_positions.end();
}

bool GameState::IsSquareSeen(const Point& square) const {
  if (0 <= square.x && square.x < map->GetSize().x &&
      0 <= square.y && square.y < map->GetSize().y) {
    return seen[square.x][square.y];
  }
  return false;
}

Sprite* GameState::SpriteAt(const Point& square) const {
  return sprite_positions.at(square);
}

void GameState::RecomputePlayerVision() {
  const int radius = player->creature->stats.vision_radius;
  player_vision.reset(new FieldOfVision(*map, player->square, radius));
  for (int x = -radius; x <= radius; x++) {
    for (int y = -radius; y <= radius; y++) {
      const Point square = player->square + Point(x, y);
      if (player_vision->IsSquareVisible(square, radius)) {
        seen[square.x][square.y] = true;
      }
    }
  }
}

}  // namespace engine
}  // namespace babel
