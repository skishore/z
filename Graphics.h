#ifndef BABEL_GRAPHICS_H__
#define BABEL_GRAPHICS_H__

#include "Engine.h"
#include "InputHandler.h"
#include "SpriteGraphics.h"

namespace babel {

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

}  // namespace babel

#endif  // BABEL_GRAPHICS_H__
