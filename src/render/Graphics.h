#ifndef __BABEL_GRAPHICS_H__
#define __BABEL_GRAPHICS_H__

#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/point.h"
#include "engine/View.h"
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
    DrawingSurface(const Point& size);
    ~DrawingSurface();

    // size is measured in grid squares, while bounds is measured in pixels.
    const Point size_;
    const SDL_Rect bounds_;
    SDL_Surface* surface_;
  };


  void DrawInner(const engine::View& view, const Transform* transform);

  void Clear();
  void Flip();

  void DrawTiles(const engine::View& view, const Point& offset);
  void DrawSprite(const engine::SpriteView& sprite, const Point& position);
  void DrawShade(const engine::View& view, const Point& offset,
                 const Point& square, const Transform::Shade& shade);

  const InterfaceView& interface_;

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_;

  std::unique_ptr<DrawingSurface> buffer_;

  std::unique_ptr<const Image> tileset_;
  std::unique_ptr<const Image> darkened_tileset_;
  std::unique_ptr<const Image> sprites_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_GRAPHICS_H__
