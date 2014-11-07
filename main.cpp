#include <assert.h>
#include <iostream>
#include <SDL2/SDL.h>

#include "ImageCache.h"
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
  Point zone_size(kZoneScreens*kScreenWidth, kZoneScreens*kScreenHeight);
  TileMap tile_map(zone_size);
  if (!tile_map.LoadMap("world.dat")) {
    cout << "Failed to load map data!" << endl;
    return -1;
  }
  tile_map.LoadZone(Point(-1, -2));
  cout << tile_map << endl;

  skishore::ImageCache cache(SDL_PIXELFORMAT_ARGB8888);
  SDL_Surface* surface_1;
  SDL_Surface* surface_2;
  assert(cache.LoadImage("tileset.bmp", &surface_1));
  assert(cache.LoadImage("tileset.bmp", &surface_2));
  assert(surface_1 == surface_2);
  assert(surface_1 != nullptr);
  return 0;
}
