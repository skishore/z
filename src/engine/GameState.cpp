#include "engine/GameState.h"

#include <algorithm>
#include <map>

#include "base/creature.h"
#include "base/debug.h"
#include "base/util.h"
#include "dialog/dialogs.h"
#include "dialog/traps.h"
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

  // TODO(skishore): Don't assume that the player starts in room 0.
  for (int i = 1; i < map->GetRooms().size(); i++) {
    const TileMap::Room& room = map->GetRooms()[i];
    // Decide whether to spawn a trap or a group of enemies in this room.
    bool trapped = rand() % 2 == 0;
    if (trapped) {
      AddTrap(new dialog::DialogGroupTrap(room.squares));
      continue;
    }
    const int num_enemies = (rand() % 3) + 2;
    for (int j = 0; j < num_enemies; j++) {
      for (int tries = 0; tries < 10; tries++) {
        const Point square = room.GetRandomSquare();
        if (!IsSquareOccupied(square)) {
          AddNPC(new Sprite(square, mGecko));
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

void GameState::AddTrap(Trap* trap) {
  ASSERT(trap != nullptr);
  for (const Point& square : trap->GetSquares()) {
    ASSERT(!IsSquareTrapped(square));
  }
  traps.push_back(trap);
  for (const Point& square : trap->GetSquares()) {
    trap_positions[square] = trap;
  }
}

void GameState::RemoveTrap(Trap* trap) {
  ASSERT(trap != nullptr);
  const auto& it = remove(traps.begin(), traps.end(), trap);
  traps.erase(it, traps.end());
  for (const Point& square : trap->GetSquares()) {
    trap_positions.erase(square);
  }
  delete trap;
}

void GameState::MaybeTriggerTrap(const Point& square, EventHandler* handler) {
  if (!IsSquareTrapped(square)) {
    return;
  }
  Trap* trap = TrapAt(square);
  trap->Trigger(this, handler);
  RemoveTrap(trap);
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

Sprite* GameState::SpriteAt(const Point& square) const {
  return sprite_positions.at(square);
}

bool GameState::IsSquareTrapped(const Point& square) const {
  return trap_positions.find(square) != trap_positions.end();
}

Trap* GameState::TrapAt(const Point& square) const {
  return trap_positions.at(square);
}

bool GameState::IsSquareSeen(const Point& square) const {
  if (0 <= square.x && square.x < map->GetSize().x &&
      0 <= square.y && square.y < map->GetSize().y) {
    return seen[square.x][square.y];
  }
  return false;
}

void GameState::RecomputePlayerVision() {
  const int radius = player->creature->stats.vision_radius;
  player_vision.reset(new FieldOfVision(*map, player->square, radius));
  for (int x = -radius; x <= radius; x++) {
    for (int y = -radius; y <= radius; y++) {
      const Point square = player->square + Point(x, y);
      if (0 <= square.x && square.x < seen.size() &&
          0 <= square.y && square.y < seen[square.x].size() &&
          player_vision->IsSquareVisible(square, radius)) {
        seen[square.x][square.y] = true;
      }
    }
  }
}

}  // namespace engine
}  // namespace babel
