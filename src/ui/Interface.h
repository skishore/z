#ifndef __BABEL_INTERFACE_H__
#define __BABEL_INTERFACE_H__

#include "engine/Action.h"
#include "interface/Dialog.h"

namespace babel {
namespace ui {

class Interface : public interface::Dialog {
 public:
  void Clear() override;
  bool Consume(char ch, engine::Action** action, bool* redraw) override;

  bool Active() const override;
  void Draw(render::DialogRenderer* renderer) const override;

 private:
  bool active_ = false;
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_INTERFACE_H__
