#ifndef BABEL_VIEW_H__
#define BABEL_VIEW_H__

#include "constants.h"

namespace babel {

struct View {
 public:
  // TODO(skishore): Change the size of the view based on the radius.
  View(int radius) : player_position(radius, radius) {}

  // If a tile is \0, it means its value is not known.
  char tiles[NCOLS][NROWS];
  Point player_position;
};

}  // namespace babel

#endif  // BABEL_VIEW_H__
