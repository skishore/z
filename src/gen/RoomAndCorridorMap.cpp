#include "gen/RoomAndCorridorMap.h"

#include "base/debug.h"
#include "base/util.h"
#include "engine/Tileset.h"
#include "engine/TileMap.h"
#include "gen/util.h"

typedef babel::engine::TileMap::Room Room;

using babel::engine::Tileset;
using std::vector;

namespace babel {
namespace gen {
namespace {

#define MAYBE_DEBUG(...) if (verbose) { DEBUG(__VA_ARGS__); }

// The caller takes ownership of the new tileset.
Tileset* DefaultTileset() {
  return new Tileset{4 /* blocked_tile */, 5 /* default_tile */};
}

}  // namespace

RoomAndCorridorMap::RoomAndCorridorMap(const Point& size, bool verbose) {
  size_ = size;
  tileset_.reset(DefaultTileset());

  CellArray cells = ConstructArray2D<Cell>(size_, Cell::DEFAULT);
  Array2D<bool> diggable = ConstructArray2D<bool>(size_, false);

  const int min_size = 6;
  const int max_size = 8;
  const int separation = 3;
  const int tries = size_.x*size_.y/(min_size*min_size);
  int tries_left = tries;

  while (tries_left > 0) {
    const Point size{RandInt(min_size, max_size), RandInt(min_size, max_size)};
    const Room room{{RandInt(1, size_.x - size.x - 1),
                     RandInt(1, size_.y - size.y - 1)}, size};
    if (!PlaceRoom(room, separation, &cells, &diggable, &rooms_)) {
      tries_left -= 1;
    }
  }
  MAYBE_DEBUG("Placed " << IntToString(rooms_.size()) << " rooms after "
              << IntToString(tries) << " attempts.");

  MAYBE_DEBUG("Final map:" << ComputeDebugString(cells));
  tiles_.reset(ComputeTiles(*tileset_, cells));
}

}  // namespace gen
}  // namespace babel
