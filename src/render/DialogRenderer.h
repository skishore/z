#ifndef __BABEL_RENDER_DIALOG_RENDERER_H__
#define __BABEL_RENDER_DIALOG_RENDERER_H__

#include <string>
#include <vector>
#include <SDL2/SDL.h>

#include "base/lru_cache.h"
#include "render/TextRenderer.h"

namespace babel {
namespace render {
namespace dialog {

class Element;

// The parent takes owernship of the child dialog. Neither may be null.
void AddChild(Element* parent, Element* child);

// The caller takes ownership of the new elements.
Element* MakeColumnElement();
Element* MakeRowElement();
Element* MakeSpanElement(bool centered);
Element* MakeTextElement(double font_size, const std::string& text,
                         uint32_t fore=0xffffffff, uint32_t back=0x0);

}  // namespace dialog

class DialogRenderer {
 public:
  DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer);

  // Takes ownership of and destructs the dialog element.
  void Draw(dialog::Element* element, bool place_at_top);
  void DrawLines(const std::vector<std::string>& lines, bool place_at_top);

  // This Text must be used before another call to DrawText is made,
  // or else it may be evicted from the cache and freed.
  Text* DrawText(int font_size, const std::string& text);

 private:
  const SDL_Rect bounds_;
  SDL_Renderer* renderer_;
  TextRenderer text_renderer_;
  LRUCache<std::pair<int,std::string>,Text> text_cache_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_RENDER_DIALOG_RENDERER_H__
