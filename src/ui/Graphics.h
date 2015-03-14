#ifndef __BABEL_UI_GRAPHICS_H__
#define __BABEL_UI_GRAPHICS_H__

#include "engine/View.h"

namespace babel {
namespace ui {

class Graphics {
 public:
  Graphics();
  ~Graphics();
  void Redraw(const engine::View& view);

 private:
  // Sets the color to the color pair at the given color index.
  void SetColor(int color);

  int last_color_ = 0;
};

}  // namespace ui 
}  // namespace babel

#endif  // __BABEL_UI_GRAPHICS_H__
