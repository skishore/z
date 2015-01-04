#ifndef __BABEL_ANIMATION_H__
#define __BABEL_ANIMATION_H__

#include <memory>

#include "base/point.h"
#include "engine/EventHandler.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "ui/Graphics.h"

namespace babel {
namespace ui {

class Animation : public engine::EventHandler {
 public:
  // EventHandler interface methods.
  void HandleAttack(const engine::Sprite& sprite,
                    const engine::Sprite& target) override;
  void HandleMove(const engine::Sprite& sprite,
                  const Point& move) override;

  // This class takes ownership of the new view.
  void SetNextView(engine::View* view);

  // Returns true if the animation is complete.
  bool Update();

 private:
  void Commit();
  void DrawTweenFrame();

  Graphics graphics_;
  std::unique_ptr<engine::View> last_;
  std::unique_ptr<engine::View> next_;
  int frame;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_ANIMATION_H__
