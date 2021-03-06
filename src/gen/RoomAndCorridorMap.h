#ifndef __BABEL_GEN_ROOM_AND_CORRIDOR_MAP_H__
#define __BABEL_GEN_ROOM_AND_CORRIDOR_MAP_H__

#include "engine/TileMap.h"

namespace babel {
namespace gen {

class RoomAndCorridorMap : public engine::TileMap {
 public:
  RoomAndCorridorMap(const Point& size, bool verbose=false);

 private:
  bool TryBuildMap(const Point& size, bool verbose);
};

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_ROOM_AND_CORRIDOR_MAP_H__
