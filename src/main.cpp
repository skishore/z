#include "base/debug.h"
#include "engine/Engine.h"
#include "ui/Bindings.h"

int main(int argc, char** argv) {
  int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
  babel::engine::Engine engine;
  babel::ui::Bindings bindings(&engine);
  return bindings.Start();
}
