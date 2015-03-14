#ifndef __BABEL_UI_BINDINGS_H__
#define __BABEL_UI_BINDINGS_H__

#include "engine/Engine.h"
#include "ui/Graphics.h"
#include "ui/InputHandler.h"

namespace babel {
namespace ui {

class Bindings {
 public:
  Bindings();

 private:
  engine::Engine engine_;
  ui::Graphics graphics_;
  ui::InputHandler handler_;
};

}  // namespace ui 
}  // namespace babel

#endif  // __BABEL_UI_BINDINGS_H__
