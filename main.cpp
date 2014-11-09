#include <assert.h>
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

namespace {
static const Point kScreenSize(16, 16);
static const Point kZoneSize(3*kScreenSize.x, 3*kScreenSize.y);
static const int kEventsPerFrame = 16;

bool IsExitEvent(const SDL_Event& event) {
  return event.type == SDL_QUIT ||
         (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
}
}  // namespace

int main(int argc, char** argv) {
  TileMap tile_map(kZoneSize);
  if (!tile_map.LoadMap("world.dat")) {
    cout << "Failed to load map data!" << endl;
    return -1;
  }
  tile_map.LoadZone(Point(-1, -2));

  ScrollingGraphics graphics(kScreenSize, &tile_map);
  graphics.RedrawBackground();
  graphics.EraseForeground();
  graphics.Flip();

  std::chrono::milliseconds timespan(16);
  bool running = true;
  while (running) {
    SDL_Event event;
    for (int i = 0; (i < kEventsPerFrame) && SDL_PollEvent(&event); i++) {
      if (IsExitEvent(event)) {
        running = false;
      }
    }
    std::this_thread::sleep_for(timespan);
  }
  return 0;
}
