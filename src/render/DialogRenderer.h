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

struct RenderParams;

class Dialog {
 public:
  virtual ~Dialog();
  virtual void AddChild(Dialog* dialog);

  virtual void Draw(const SDL_Rect& rect, const RenderParams& params) const = 0;
  virtual int GetHeight() const = 0;

 protected:
  std::vector<Dialog*> children_;
};

// The caller takes ownership of the new dialogs.
Dialog* MakeColumnDialog();
Dialog* MakeRowDialog();
Dialog* MakeTextDialog(int font_size, const std::string& text, uint32_t color);

}  // namespace dialog

class DialogRenderer {
 public:
  DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer);

  void Draw(const dialog::Dialog& dialog, bool place_at_top);
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
