#ifndef __BABEL_GRAPHICS_H__
#define __BABEL_GRAPHICS_H__

#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "Image.h"
#include "Point.h"
#include "View.h"
#include "TextRenderer.h"

namespace babel {

class Graphics {
 public:
  Graphics(const Point& size);
  ~Graphics();

  void Clear();
  void Draw(const View& view);
  void DrawUI();
  void Flip();

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

  void DrawTexts(const std::vector<Point>& positions,
                 const std::vector<std::string>& texts,
                 const std::vector<SDL_Color>& colors);
  void DrawText(int x, int y, Direction direction,
                const std::string& text, SDL_Color color);

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

} // namespace babel

#endif  // __BABEL_GRAPHICS_H__
