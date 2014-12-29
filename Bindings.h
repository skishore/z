#ifndef BABEL_BINDINGS_H__
#define BABEL_BINDINGS_H__

#include "Engine.h"
#include "GameLoop.h"
#include "Graphics.h"
#include "InputHandler.h"

namespace babel {

class Bindings : GameLoop::Updatable {
 public:
  Bindings(Engine* engine);
  int Start();
  bool Update(double frame_rate) override;

 private:
  void Redraw();

  Engine* engine_;
  Graphics graphics_;
  InputHandler input_;
};

}  // namespace babel

#endif  // BABEL_BINDINGS_H__
