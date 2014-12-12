#ifndef __SKISHORE_SCROLLING_GRAPHICS_H__
#define __SKISHORE_SCROLLING_GRAPHICS_H__

#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "Image.h"
#include "ImageCache.h"
#include "Point.h"
#include "Sprite.h"
#include "TextRenderer.h"
#include "TileMap.h"

namespace skishore {

class ScrollingGraphics {
 public:
  ScrollingGraphics(const Point& size, const TileMap& map);
  ~ScrollingGraphics();

  ImageCache* GetImageCache() { return &cache_; };
  void DrawStatusMessage(const std::string& message);

  // CenterCamera centers the camera on the given pixel position.
  void CenterCamera(const Point& position);
  void DrawSprite(const Sprite& sprite);
  void DrawSpriteText(const Sprite& sprite);
  void DrawUI(const std::vector<std::string>& lines);

  // RedrawBackground is called by CenterCamera if the camera is out-of-bounds.
  // EraseForeground copies the background currently in view to the foreground,
  // while Flip copes the foreground to the screen.
  void RedrawBackground();
  void EraseForeground();
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

  // map_ is passed in the constructor and owned by the caller.
  const TileMap& map_;
  ImageCache cache_;

  // These three SDL structures are for drawing to actual video memory.
  SDL_Window* window_;
  SDL_Renderer* renderer_;
  SDL_Texture* texture_;

  // The screen is backed by two buffers. The foreground is the same size as
  // the screen and is the buffer on which we assemble the next frame.
  //
  // The background is larger than the screen and only has background tiles -
  // that is, it does not have any sprites on it. We use it to scroll the map.
  std::unique_ptr<DrawingSurface> foreground_;
  std::unique_ptr<DrawingSurface> background_;

  std::unique_ptr<TextRenderer> text_renderer_;

  // To maintain the background, a ScrollingGraphics instance tracks the offset
  // (in grid squares) of the background within the map and the position of
  // the camera (in pixels) of the foreground within the background.
  // We also store a rect that must contain a sprite for it to be "centered".
  Point background_offset_;
  Point camera_;
  Point position_offset_;
  SDL_Rect centered_;

  std::unique_ptr<const Image> tileset_;
};

} // namespace skishore

#endif  // __SKISHORE_SCROLLING_GRAPHICS_H__
