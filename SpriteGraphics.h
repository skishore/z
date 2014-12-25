#ifndef __SKISHORE_SPRITE_GRAPHICS_H__
#define __SKISHORE_SPRITE_GRAPHICS_H__

#include <SDL2/SDL.h>

#include "Image.h"
#include "ImageCache.h"
#include "Point.h"
#include "TextRenderer.h"

namespace skishore {

class SpriteGraphics {
 public:
  SpriteGraphics(const Point& size);
  ~SpriteGraphics();

  void DrawTile(int x, int y, char tile);
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

  ImageCache cache_;

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_;

  std::unique_ptr<DrawingSurface> buffer_;
  std::unique_ptr<const Image> tileset_;
  std::unique_ptr<const Image> player_;
  std::unique_ptr<TextRenderer> text_renderer_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_GRAPHICS_H__
