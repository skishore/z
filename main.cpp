#include <iostream>

#include "Engine.h"
#include "Point.h"

using std::cout;
using std::endl;
using skishore::Engine;
using skishore::Point;

namespace {
static const Point kScreenSize(16, 16);
static const int kFrameRate = 60;
}  // namespace

int main(int argc, char** argv) {
  Engine(kFrameRate, kScreenSize);
  return 0;
}
