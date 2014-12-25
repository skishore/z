#include "debug.h"
#include "Engine.h"
#include "Graphics.h"

using babel::Engine;

int main(int argc, char** argv) {
  babel::RegisterCrashHandlers(argv[0]);
  babel::Engine engine;
  babel::Graphics graphics(&engine);
  return graphics.Start();
}
