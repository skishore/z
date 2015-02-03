#ifndef __BABEL_DIALOG_RENDERER_H__
#define __BABEL_DIALOG_RENDERER_H__

#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "render/TextRenderer.h"

namespace babel {
namespace render {

class DialogRenderer {
 public:
  DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer);

  void DrawLines(const std::vector<std::string>& lines, bool place_at_top);

 private:
  const SDL_Rect bounds_;
  SDL_Renderer* renderer_;
  TextRenderer text_renderer_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_DIALOG_RENDERER_H__
