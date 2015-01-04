#ifndef __BABEL_ANIMATION_H__
#define __BABEL_ANIMATION_H__

#include <memory>

#include "base/point.h"
#include "engine/EventHandler.h"
#include "engine/View.h"
#include "ui/Graphics.h"

namespace babel {
namespace ui {

class Animation : public engine::EventHandler {
 public:
  // EventHandler interface methods.
  void HandleAttack(const Point& source, const Point& target) override;
  void HandleMove(const Point& source, const Point& target) override;

  // This class takes ownership of the new view.
  void SetNextView(engine::View* view);

  // Returns true if the animation is complete.
  bool Update();

 private:
  Graphics graphics_;
  std::unique_ptr<engine::View> last_;
  std::unique_ptr<engine::View> next_;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_ANIMATION_H__
