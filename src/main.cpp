#ifdef EMSCRIPTEN
  #include <emscripten.h>
  #include "bindings.h"
#else
  #include "ui/Bindings.h"
#endif  // EMSCRIPTEN

#include "base/debug.h"
#include "engine/Engine.h"

int main(int argc, char** argv) {
  int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
  #ifdef EMSCRIPTEN
    emscripten_exit_with_live_runtime();
  #else
    // The game starts immediately when bindings is constructed.
    babel::ui::Bindings bindings;
    return 0;
  #endif  // EMSCRIPTEN
}
