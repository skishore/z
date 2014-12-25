#ifndef ROGUE_GRAPHICS_H__
#define ROGUE_GRAPHICS_H__

#include "Engine.h"
#include "InputHandler.h"
#include "SpriteGraphics.h"

namespace skishore {

class Graphics {
 public:
  Graphics(Engine* engine);
  int Start();

 private:
  void Redraw();

  Engine* engine_;
  InputHandler input_;
  SpriteGraphics sprite_graphics_;
};

}  // namespace skishore

#endif  // ROGUE_GRAPHICS_H__
