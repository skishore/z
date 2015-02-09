#ifndef __BABEL_RENDER_DIALOG_RENDERER_H__
#define __BABEL_RENDER_DIALOG_RENDERER_H__

#include <list>
#include <map>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/lru_cache.h"
#include "render/TextRenderer.h"

namespace babel {
namespace render {

class DialogRenderer {
 public:
  DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer);

  void DrawLines(const std::vector<std::string>& lines, bool place_at_top);

 private:
  Text* DrawText(const std::string& text);

  const SDL_Rect bounds_;
  SDL_Renderer* renderer_;
  TextRenderer text_renderer_;
  LRUCache<std::string,Text> text_cache_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_RENDER_DIALOG_RENDERER_H__
