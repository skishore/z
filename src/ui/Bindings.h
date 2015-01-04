#ifndef BABEL_BINDINGS_H__
#define BABEL_BINDINGS_H__

#include "engine/Engine.h"
#include "ui/Animation.h"
#include "ui/GameLoop.h"
#include "ui/InputHandler.h"

namespace babel {
namespace ui {

class Bindings : GameLoop::Updatable {
 public:
  Bindings();

  int Start();
  bool Update(double frame_rate) override;

 private:
  void Redraw();

  Animation animation_;
  engine::Engine engine_;
  InputHandler input_;
};

}  // namespace ui
}  // namespace babel

#endif  // BABEL_BINDINGS_H__
