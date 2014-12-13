#ifndef ROGUE_VIEW_H__
#define ROGUE_VIEW_H__

#include "constants.h"

struct View {
 public:
  // If a tile is \0, it means its value is not known.
  char tiles[NCOLS][NROWS];
  Point player_position;
};

#endif  // ROGUE_VIEW_H__
