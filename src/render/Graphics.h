#ifndef __BABEL_RENDER_GRAPHICS_H__
#define __BABEL_RENDER_GRAPHICS_H__

#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/point.h"
#include "engine/View.h"
#include "interface/Dialog.h"
#include "render/DialogRenderer.h"
#include "render/Image.h"
#include "render/Transform.h"

namespace babel {
namespace render {

class Graphics {
 public:
  Graphics(int radius, const interface::Dialog& dialog);
  ~Graphics();

  void Draw(const engine::View& view);
  void Draw(const engine::View& view, const Transform& transform);

 private:
  void DrawInner(const engine::View& view, const Transform* transform);
  void DrawTiles(const engine::View& view, const Point& offset);
  void DrawShade(const engine::View& view, const Point& offset,
                 const Point& square, const Transform::Shade& shade);

  const interface::Dialog& dialog_;

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;

  SDL_Rect bounds_;
  std::unique_ptr<DialogRenderer> dialog_renderer_;
  std::unique_ptr<const Image> tileset_;
  std::unique_ptr<const Image> darkened_tileset_;
  std::unique_ptr<const Image> sprites_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_RENDER_GRAPHICS_H__
