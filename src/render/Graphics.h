#ifndef __BABEL_GRAPHICS_H__
#define __BABEL_GRAPHICS_H__

#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/point.h"
#include "engine/View.h"
#include "render/DialogRenderer.h"
#include "render/Image.h"
#include "render/Transform.h"

namespace babel {
namespace render {

class InterfaceView {
 public:
  virtual bool HasLines() const = 0;
  virtual std::vector<std::string> GetLines() const = 0;
};

class Graphics {
 public:
  Graphics(int radius, const InterfaceView& interface);
  ~Graphics();

  void Draw(const engine::View& view);
  void Draw(const engine::View& view, const Transform& transform);

 private:
  class DrawingSurface {
   public:
    DrawingSurface(const Point& size, SDL_Renderer* renderer);
    ~DrawingSurface();

    // size is measured in grid squares, while bounds is measured in pixels.
    const Point size;
    const SDL_Rect bounds;
    SDL_Texture* texture;
  };


  void DrawInner(const engine::View& view, const Transform* transform);
  void DrawTiles(const engine::View& view, const Point& offset);
  void DrawShade(const engine::View& view, const Point& offset,
                 const Point& square, const Transform::Shade& shade);

  const InterfaceView& interface_;

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;

  std::unique_ptr<DrawingSurface> buffer_;
  std::unique_ptr<DialogRenderer> dialog_renderer_;
  std::unique_ptr<const Image> tileset_;
  std::unique_ptr<const Image> darkened_tileset_;
  std::unique_ptr<const Image> sprites_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_GRAPHICS_H__
