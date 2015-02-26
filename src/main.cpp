#include "base/debug.h"

int main(int argc, char** argv) {
  int seed = time(nullptr);
  DEBUG("Using seed " << seed);
  srand(seed);
  babel::RegisterCrashHandlers(argv[0]);
}
