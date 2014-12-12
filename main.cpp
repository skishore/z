#include <iostream>

#include "constants.h"
#include "Engine.h"
#include "Point.h"

using std::cout;
using std::endl;
using skishore::Engine;
using skishore::Point;

namespace {
static const Point kScreenSize(16, 16);
}  // namespace

int main(int argc, char** argv) {
  Engine(skishore::kFrameRate, kScreenSize);
  return 0;
}
