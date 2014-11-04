#include <iostream>

#include "Point.h"
#include "TileMap.h"

using std::cout;
using std::endl;
using skishore::Point;
using skishore::TileMap;

static const int kZoneScreens = 3;
static const int kScreenWidth = 16;
static const int kScreenHeight = 16;

int main(int argc, char** argv) {
  cout << "Loading...";
  Point zone_size(kZoneScreens*kScreenWidth, kZoneScreens*kScreenHeight);
  TileMap tile_map(zone_size);
  bool success = tile_map.LoadMap("world.dat");
  cout << (success ? "success." : "failed!") << endl;
  return 0;
}
