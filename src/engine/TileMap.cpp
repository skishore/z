#include "engine/TileMap.h"

#include <fstream>
#include <iostream>

#include "base/debug.h"

using std::string;
using std::vector;

namespace babel {
namespace engine {

namespace {
void CheckSkipToken(const string& expected, const string& actual) {
  ASSERT(expected == actual,
         "Expected: \"" << expected << "\", actual: \"" << actual << "\"");
}
}  // namespace

void TileMap::LoadMap(const string& filename) {
  std::ifstream file("data/" + filename, std::ios::in | std::ios::binary);
  ASSERT(file.is_open(), "Failed to open " << filename);

  // Read the map's height and width.
  string skip_token;
  file >> skip_token >> map_dimensions_.x >> skip_token >> map_dimensions_.y;
  const int map_size = map_dimensions_.x*map_dimensions_.y;

  // Read the default tile.
  int map_default_tile;
  file >> skip_token >> map_default_tile;
  CheckSkipToken("map_default_tile:", skip_token);
  map_default_tile_ = map_default_tile;

  // Read the starting square.
  file >> skip_token >> starting_square_.x >> starting_square_.y;
  CheckSkipToken("starting_square:", skip_token);

  // Read the list of rooms.
  int num_rooms;
  file >> skip_token >> num_rooms;
  CheckSkipToken("num_rooms:", skip_token);
  rooms_.resize(num_rooms);
  for (int i = 0; i < num_rooms; i++) {
    file >> skip_token >> rooms_[i].position.x >> rooms_[i].position.y
         >> rooms_[i].size.x >> rooms_[i].size.y;
    CheckSkipToken("room:", skip_token);
  }

  // Read the actual map data.
  file >> skip_token;
  CheckSkipToken("tiles:", skip_token);
  file.seekg(1, std::ios::cur);
  map_tiles_.reset(new Tile[map_size]);
  file.read((char*)map_tiles_.get(), map_size);
  ASSERT(file.gcount() == map_size,
         "Expected to read " << map_size << " characters from "
         << filename << ", but only read " << file.gcount());
}

Tile TileMap::GetMapTile(const Point& square) const {
  if (0 <= square.x && square.x < map_dimensions_.x &&
      0 <= square.y && square.y < map_dimensions_.y) {
    return map_tiles_[square.x*map_dimensions_.y + square.y];
  }
  return map_default_tile_;
}

bool TileMap::IsSquareBlocked(const Point& square) const {
  return GetMapTile(square) > 3;
}

}  // namespace engine
} // namespace babel
