#include "debug.h"
#include "Engine.h"
#include "Graphics.h"

using skishore::Engine;

int main(int argc, char** argv) {
  skishore::RegisterCrashHandlers(argv[0]);
  skishore::Engine engine;
  skishore::Graphics graphics(&engine);
  return graphics.Start();
}
