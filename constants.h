// This file contains any constants that are used in multiple places in code.
// If a constant is only used in one cpp file, it should be defined there.
//
// Each constant here may indicate the existence of some leaky abstraction...

#ifndef __SKISHORE_CONSTANTS_H__
#define __SKISHORE_CONSTANTS_H__

#include <SDL2/SDL.h>

#include "Point.h"

namespace skishore {

// The side length of each grid square, in pixels.
static const int kGridSize = 16;

// All movement calculations are done in ticks.
static const int kTicksPerPixel = 1024;
static const int kGridTicks = kGridSize*kTicksPerPixel;
static const int kPlayerSpeed = 1.4*kTicksPerPixel;

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

static SDL_Keycode kDirectionKey[4] = {
  SDLK_UP,
  SDLK_RIGHT,
  SDLK_DOWN,
  SDLK_LEFT
};

}

#endif  // __SKISHORE_CONSTANTS_H__
