#ifndef BABEL_CONSTANTS_H__
#define BABEL_CONSTANTS_H__

#include "base/point.h"

namespace babel {

enum Direction {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3
};

inline Direction OppositeDirection(Direction dir) {
  return (Direction)(dir ^ 2);
}

enum Orientation {
  TT = 0,
  TR = 1,
  RR = 2,
  BR = 3,
  BB = 4,
  BL = 5,
  LL = 6,
  TL = 7
};

} //  namespace babel

#endif  // BABEL_CONSTANTS_H__
