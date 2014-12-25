#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include "constants.h"

namespace babel {

struct View {
 public:
  // If a tile is \0, it means its value is not known.
  char tiles[NCOLS][NROWS];
  Point player_position;
};

}  // namespace babel

#endif  // BABEL_VIEW_H__
