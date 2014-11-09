#include <chrono>
#include <iostream>
#include <thread>
#include <SDL2/SDL.h>

#include "ScrollingGraphics.h"
#include "Point.h"
#include "TileMap.h"

using std::cout;
using std::endl;
using skishore::Point;
using skishore::ScrollingGraphics;
using skishore::TileMap;

static const Point kScreenSize(16, 16);
static const Point kZoneSize(3*kScreenSize.x, 3*kScreenSize.y);

int main(int argc, char** argv) {
  TileMap tile_map(kZoneSize);
  if (!tile_map.LoadMap("world.dat")) {
    cout << "Failed to load map data!" << endl;
    return -1;
  }
  tile_map.LoadZone(Point(-1, -2));
  cout << tile_map << endl;

  ScrollingGraphics graphics(kScreenSize, &tile_map);
  while (true) {
    std::chrono::milliseconds timespan(1000);
    std::this_thread::sleep_for(timespan);
  }
  return 0;
}
