#ifndef BABEL_CONSTANTS_H__
#define BABEL_CONSTANTS_H__

#include "base/point.h"

namespace babel {

static const int kScreenRadius = 10;

enum Direction {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3
};

inline Direction OppositeDirection(Direction dir) {
  return (Direction)(dir ^ 2);
}

} //  namespace babel

#endif  // BABEL_CONSTANTS_H__