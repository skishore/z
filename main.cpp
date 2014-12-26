#include "debug.h"
#include "Bindings.h"
#include "Engine.h"

using babel::Engine;

int main(int argc, char** argv) {
  babel::RegisterCrashHandlers(argv[0]);
  babel::Engine engine;
  babel::Bindings bindings(&engine);
  return bindings.Start();
}
