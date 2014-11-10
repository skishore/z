#include <assert.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <SDL2/SDL.h>

#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif

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
static const int kEventsPerFrame = 16;

static const int kFrameRate = 60;
static const int kMillisecondsPerFrame = 1000/kFrameRate;

bool IsExitEvent(const SDL_Event& event) {
  return event.type == SDL_QUIT ||
         (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
}

bool Update() {
  SDL_Event event;
  for (int i = 0; (i < kEventsPerFrame) && SDL_PollEvent(&event); i++) {
    if (IsExitEvent(event)) {
      return true;
    }
  }
  return false;
}

#ifdef EMSCRIPTEN
void VoidUpdate() {
  Update();
}
#else
void GameLoop() {
  std::chrono::milliseconds timespan(kMillisecondsPerFrame);
  while (true) {
    if (Update()) break;
    std::this_thread::sleep_for(timespan);
  }
}
#endif

}  // namespace

int main(int argc, char** argv) {
  TileMap tile_map;
  if (!tile_map.LoadMap("world.dat")) {
    cout << "Failed to load map data!" << endl;
    return -1;
  }

  ScrollingGraphics graphics(kScreenSize, &tile_map);
  graphics.CenterCamera(Point(6, 5));
  graphics.EraseForeground();
  graphics.Flip();

  #ifdef EMSCRIPTEN
  emscripten_set_main_loop(VoidUpdate, kFrameRate, true);
  #else
  GameLoop();
  #endif
  return 0;
}
