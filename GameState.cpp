#include "creature.h"
#include "debug.h"
#include "GameState.h"
#include "Sprite.h"

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

void GameState::PushLogLine(const string& line) {
  log.push_back(line);
  if (log.size() > kMaxLogSize) {
    log.pop_front();
  }
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
      if (player_vision->IsSquareVisible(square)) {
        seen[square.x][square.y] = true;
      }
    }
  }
}

}  // namespace babel
