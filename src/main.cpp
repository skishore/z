#include "base/debug.h"
#include "ui/Bindings.h"

int main(int argc, char** argv) {
  int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
  babel::ui::Bindings bindings(argc > 1);
  return bindings.Start();
}
