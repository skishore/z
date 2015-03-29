#include "gen/RoomAndCorridorMap.h"

#include "engine/Tileset.h"
#include "engine/TileMap.h"
#include "gen/util.h"

using babel::engine::Tileset;
using std::vector;

namespace babel {
namespace gen {
namespace {

// The caller takes ownership of the new tileset.
Tileset* DefaultTileset() {
  return new Tileset{4 /* blocked_tile */, 5 /* default_tile */};
}

}  // namespace

RoomAndCorridorMap::RoomAndCorridorMap(const Point& size) {
  size_ = size;
  tileset_.reset(DefaultTileset());

  vector<vector<bool>> blocked = Construct2DArray(size_, true);
  vector<vector<bool>> diggable = Construct2DArray(size_, false);
}

}  // namespace gen
}  // namespace babel
