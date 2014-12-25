#ifndef ROGUE_VIEW_H__
#define ROGUE_VIEW_H__

#include "constants.h"

namespace skishore {

struct View {
 public:
  // If a tile is \0, it means its value is not known.
  char tiles[NCOLS][NROWS];
  Point player_position;
};

}  // namespace skishore

#endif  // ROGUE_VIEW_H__
