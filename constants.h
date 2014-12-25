#ifndef ROGUE_CONSTANTS_H__
#define ROGUE_CONSTANTS_H__

#include "Point.h"

namespace skishore {

static const int NCOLS = 24;
static const int NROWS = 24;
// TODO(skishore): Switch everyone over to kWindowSize.
static const Point kWindowSize(NCOLS, NROWS);

enum Direction {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3
};

inline Direction OppositeDirection(Direction dir) {
  return (Direction)(dir ^ 2);
}

static const Point kShift[4] = {
  Point(0, -1),
  Point(1, 0),
  Point(0, 1),
  Point(-1, 0)
};

static char kDirectionKey[4] = {'k', 'l', 'j', 'h'};

} //  namespace skishore

#endif  // ROGUE_CONSTANTS_H__
