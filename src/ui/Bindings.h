#ifndef BABEL_BINDINGS_H__
#define BABEL_BINDINGS_H__

#include <memory>

#include "engine/Engine.h"
#include "ui/Animation.h"
#include "ui/GameLoop.h"
#include "ui/Graphics.h"
#include "ui/InputHandler.h"

namespace babel {
namespace ui {

class Bindings : GameLoop::Updatable {
 public:
  Bindings(bool verbose);

  int Start();
  bool Update(double frame_rate) override;

 private:
  void Reset();
  void Redraw();

  bool verbose_;
  Graphics graphics_;
  InputHandler input_;
  std::unique_ptr<Animation> animation_;
  std::unique_ptr<engine::Engine> engine_;
};

}  // namespace ui
}  // namespace babel

#endif  // BABEL_BINDINGS_H__
