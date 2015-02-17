#ifndef __BABEL_UI_BINDINGS_H__
#define __BABEL_UI_BINDINGS_H__

#include <memory>

#include "engine/Engine.h"
#include "render/Graphics.h"
#include "ui/Animation.h"
#include "ui/GameLoop.h"
#include "ui/InputHandler.h"
#include "ui/Interface.h"

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

  render::Graphics graphics_;
  InputHandler input_;
  Interface interface_;
  std::unique_ptr<Animation> animation_;
  std::unique_ptr<engine::Engine> engine_;
};

}  // namespace ui
}  // namespace babel

#endif  // __BABEL_UI_BINDINGS_H__
