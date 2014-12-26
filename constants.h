#ifndef BABEL_CONSTANTS_H__
#define BABEL_CONSTANTS_H__

#include <map>

#include "Point.h"

using std::map;

namespace babel {

static const int kScreenRadius = 15;
static const int NCOLS = 2*kScreenRadius + 1;
static const int NROWS = 2*kScreenRadius + 1;

enum Direction {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3
};

inline Direction OppositeDirection(Direction dir) {
  return (Direction)(dir ^ 2);
}

static const map<char,Point> kShift = {
  {'h', Point(-1, 0)},
  {'j', Point(0, 1)},
  {'k', Point(0, -1)},
  {'l', Point(1, 0)},
  {'y', Point(-1, -1)},
  {'u', Point(1, -1)},
  {'b', Point(-1, 1)},
  {'n', Point(1, 1)},
  {'.', Point(0, 0)}
};

} //  namespace babel

#endif  // BABEL_CONSTANTS_H__
