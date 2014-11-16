// This file contains any constants that are used in multiple places in code.
// If a constant is only used in one cpp file, it should be defined there.
//
// Each constant here may indicate the existence of some leaky abstraction...

#ifndef __SKISHORE_CONSTANTS_H__
#define __SKISHORE_CONSTANTS_H__

#include "Point.h"

namespace skishore {

// The side length of each grid square, in pixels.
static const int kGridSize = 16;

enum Direction {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3
};

static const Point kShift[4] = {
  Point(0, -1),
  Point(1, 0),
  Point(0, 1),
  Point(-1, 0)
};

}

#endif  // __SKISHORE_CONSTANTS_H__
