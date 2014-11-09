#ifndef __SKISHORE_SCROLLING_GRAPHICS_H__
#define __SKISHORE_SCROLLING_GRAPHICS_H__

#include <SDL2/SDL.h>

#include "ImageCache.h"
#include "Point.h"

namespace skishore {

class Sprite;
class TileMap;

class ScrollingGraphics {
 public:
  ScrollingGraphics(const Point& size, const TileMap* map);
  ~ScrollingGraphics();

  // CenterCamera centers the camera on the given grid square.
  // EraseForeground copies the background currently in view to the foreground,
  // while Flip copes the foreground to the screen.
  void CenterCamera(const Point& square);
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

  // RedrawBackground is called by CenterCamera if the camera is out-of-bounds.
  void RedrawBackground();

  // map_ is passed in the constructor and owned by the caller.
  const TileMap* map_;
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

  // To maintain the background, a ScrollingGraphics instance tracks the offset
  // (in grid squares) of the background within the map and the position of
  // the camera (in pixels) of the foreground within the background.
  Point background_offset_;
  Point camera_;

  std::unique_ptr<Sprite> tileset_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
