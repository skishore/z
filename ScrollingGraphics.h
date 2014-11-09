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

  // RedrawBackground should be called when a new zone is loaded to draw it
  // to the background buffer. EraseForeground copies the background in
  // view to the foreground, while Flip copes the foreground to the screen.
  void RedrawBackground();
  void EraseForeground();
  void Flip();

 private:
  // size_ is measure in grid squares, so it is the responsibility of this
  // class to convert it to actual pixel dimensions.
  // map_ is passed in the constructor and owned by the caller.
  const Point size_;
  const Point zone_size_;
  const TileMap* map_;
  const SDL_Rect bounds_;
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
  SDL_Surface* foreground_;
  SDL_Surface* background_;

  std::unique_ptr<Sprite> tileset_;
  Point camera_;
};

} // namespace skishore

#endif  // __SKISHORE_SPRITE_H__
