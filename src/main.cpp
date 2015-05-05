#ifdef EMSCRIPTEN
#include <emscripten.h>
#else
#include "gen/RoomAndCorridorMap.h"
#endif  // EMSCRIPTEN

#include "base/debug.h"
#include "bindings.h"
#include "engine/Engine.h"

int main(int argc, char** argv) {
  const int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
  #ifdef EMSCRIPTEN
  emscripten_exit_with_live_runtime();
  #else
  const babel::Point kMapSize(64, 64);
  babel::gen::RoomAndCorridorMap map(kMapSize, true /* debug */);
  #endif  // EMSCRIPTEN
}
