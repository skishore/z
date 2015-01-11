#ifndef __BABEL_GRAPHICS_H__
#define __BABEL_GRAPHICS_H__

#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/point.h"
#include "engine/View.h"
#include "ui/Image.h"
#include "ui/TextRenderer.h"
#include "ui/Transform.h"

namespace babel {
namespace ui {

class Graphics {
 public:
  Graphics();
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

  void DrawTexts(const std::vector<Point>& positions,
                 const std::vector<std::string>& texts,
                 const std::vector<SDL_Color>& colors);
  void DrawText(const Point& position, Direction direction,
                const std::string& text, SDL_Color color);

  void DrawLog(const std::vector<std::string>& log);
  void DrawStatus(const engine::StatusView& status);
  void DrawDialogBox(const std::vector<std::string>& lines, bool place_at_top);

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_;

  std::unique_ptr<DrawingSurface> buffer_;
  std::unique_ptr<TextRenderer> text_renderer_;

  std::unique_ptr<const Image> tileset_;
  std::unique_ptr<const Image> darkened_tileset_;
  std::unique_ptr<const Image> sprites_;
};

} // namespace ui 
} // namespace babel

#endif  // __BABEL_GRAPHICS_H__
