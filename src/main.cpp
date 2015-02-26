#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif  // EMSCRIPTEN

#include "base/debug.h"
#include "bindings.h"
#include "engine/Engine.h"

int main(int argc, char** argv) {
  int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
  #ifdef EMSCRIPTEN
  babel::gEngine.reset(new babel::engine::Engine);
  emscripten_exit_with_live_runtime();
  #endif  // EMSCRIPTEN
}
