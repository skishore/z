#include <algorithm>
#include <map>

#include "creature.h"
#include "debug.h"
#include "util.h"
#include "GameState.h"
#include "Sprite.h"

using std::map;
using std::max;
using std::string;
using std::vector;

namespace babel {

namespace {
static const int kMaxLogSize = 24;
}

GameState::GameState(const string& map_file) {
  map.LoadMap(map_file);
  seen = vector<vector<bool>>(
      map.GetSize().x, vector<bool>(map.GetSize().y, false));
  player = new Sprite(map.GetStartingSquare(), kPlayerType);
  AddNPC(player);
  RecomputePlayerVision();

  for (int i = 1; i < map.GetRooms().size(); i++) {
    const TileMap::Room& room = map.GetRooms()[i];
    const int num_enemies = (room.size.x + room.size.y)/2 - 3;
    for (int j = 0; j < num_enemies; j++) {
      while (true) {
        Point square = room.position;
        square.x += rand() % room.size.x;
        square.y += rand() % room.size.y;
        if (!IsSquareOccupied(square)) {
          int type = (rand() % (kCreatures.size() - 1)) + 1;
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
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(!IsSquareOccupied(sprite->square),
         "Adding sprite at occupied square " << sprite->square);
  sprites.push_back(sprite);
  sprite_positions[sprite->square] = sprite;
}

void GameState::RemoveNPC(Sprite* sprite) {
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(!sprite->IsPlayer(), "Removing player!");
  const auto& it = remove(sprites.begin(), sprites.end(), sprite);
  sprites.erase(it, sprites.end());
  sprite_positions.erase(sprite->square);
  delete sprite;
}

void GameState::MoveSprite(Sprite* sprite, const Point& move) {
  if (move.x == 0 && move.y == 0) {
    return;
  }
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(sprite_positions[sprite->square] == sprite,
         "Integrity error: unexpected sprite at " << sprite->square);
  Point new_square = sprite->square + move;
  ASSERT(!IsSquareOccupied(new_square),
         "Moving sprite to occupied square " << new_square);
  sprite_positions.erase(sprite->square);
  sprite_positions[new_square] = sprite;
  sprite->square = new_square;

  if (sprite == player) {
    RecomputePlayerVision();
  }
}

void GameState::AddLogLine(const string& line) {
  log.push_back(line);
  if (log.size() > kMaxLogSize) {
    log.pop_front();
    log_index = max(log_index - 1, 0);
  }
}

void GameState::CoalesceLog() {
  if (log_index == log.size()) {
    return;
  }
  std::map<string,int> counts;
  string line;
  for (int i = log_index; i < log.size(); i++) {
    counts[log[i]] += 1;
  }
  for (int i = log_index; i < log.size(); i++) {
    const int count = counts[log[i]];
    if (count > 0) {
      line += (line.empty() ? "" : " ") + log[i];
      if (count > 1) {
        line += " (x" + IntToString(count) + ")";
      }
      counts[log[i]] = 0;
    }
  }
  while(log.size() > log_index) {
    log.pop_back();
  }
  log.push_back(line);
  log_index += 1;
}

bool GameState::IsSquareOccupied(const Point& square) const {
  return sprite_positions.find(square) != sprite_positions.end();
}

bool GameState::IsSquareSeen(const Point& square) const {
  if (0 <= square.x && square.x < map.GetSize().x &&
      0 <= square.y && square.y < map.GetSize().y) {
    return seen[square.x][square.y];
  }
  return false;
}

Sprite* GameState::SpriteAt(const Point& square) const {
  return sprite_positions.at(square);
}

void GameState::RecomputePlayerVision() {
  const int radius = player->creature.stats.vision_radius;
  player_vision.reset(new FieldOfVision(map, player->square, radius));
  for (int x = -radius; x <= radius; x++) {
    for (int y = -radius; y <= radius; y++) {
      const Point square = player->square + Point(x, y);
      if (player_vision->IsSquareVisible(square, radius)) {
        seen[square.x][square.y] = true;
      }
    }
  }
}

}  // namespace babel
