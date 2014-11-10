#include <assert.h>
#include <chrono>
#include <iostream>
#include <sys/time.h>
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

std::unique_ptr<ScrollingGraphics> graphics;

bool IsExitEvent(const SDL_Event& event) {
  return event.type == SDL_QUIT ||
         (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE);
}

bool Update() {
  // All times are stored ticks, which are microseconds.
  static long long min_time = 0, cur_time = 0, last_second = 0;
  static const int kTicksPerSecond = 1000000;
  static int frames = 0;
  static timeval time;
  gettimeofday(&time, NULL);
  cur_time = time.tv_sec*kTicksPerSecond+ time.tv_usec - min_time;
  if (cur_time < 0) {
    min_time += cur_time;
  }

  graphics->RedrawBackground();
  graphics->EraseForeground();
  graphics->Flip();

  SDL_Event event;
  for (int i = 0; (i < kEventsPerFrame) && SDL_PollEvent(&event); i++) {
    if (IsExitEvent(event)) {
      return true;
    }
  }

  if (cur_time > last_second + kTicksPerSecond) {
    if (frames > 0) {
      double fps = 1.0*frames*kTicksPerSecond/(cur_time - last_second);
      cout << "FPS: " << fps << endl;
    }
    last_second = cur_time;
    frames = 0;
  }
  frames++;
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

  graphics.reset(new ScrollingGraphics(kScreenSize, tile_map));
  graphics->RedrawBackground();
  graphics->EraseForeground();
  graphics->Flip();

  #ifdef EMSCRIPTEN
  emscripten_set_main_loop(VoidUpdate, 0, true);
  #else
  GameLoop();
  #endif
  return 0;
}
