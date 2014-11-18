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

// Movement speed constants.
static const double kPlayerSpeed = 1.6;

// Kinematic constraint constants.
static const double kKinematicSeparation = 32.0;
static const double kKinematicSensitivity = 8.0;
static const double kKinematicMinDist = 2.0;
static const double kKinematicPlayerForce  = 1.1;
static const double kKinematicBackoff = 0.4;

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
