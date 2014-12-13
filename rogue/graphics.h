#ifndef ROGUE_GRAPHICS_H__
#define ROGUE_GRAPHICS_H__

#include "engine.h"

class Graphics {
 public:
  Graphics(Engine* engine);
  int Start();

 private:
  void Redraw() const;

  Engine* engine_;
};

#endif  // ROGUE_GRAPHICS_H__
