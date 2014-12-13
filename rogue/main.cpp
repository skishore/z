#include "engine.h"
#include "graphics.h"

int main() {
  Engine engine;
  Graphics graphics(&engine);
  return graphics.Start();
}
