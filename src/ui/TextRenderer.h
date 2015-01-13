#ifndef __BABEL_TEXT_RENDERER_H__
#define __BABEL_TEXT_RENDERER_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "base/point.h"

namespace babel {
namespace ui {

static const SDL_Color kBlack{0, 0, 0};
static const SDL_Color kWhite{255, 255, 255};

namespace font {
class Font;
}  // namespace font

class TextRenderer {
 public:
  TextRenderer(const SDL_Rect& bounds, SDL_Surface* target);
  ~TextRenderer();

  void DrawText(
      const std::string& font_name, int font_size,
      const std::string& text, const SDL_Rect& rect,
      const SDL_Color color=kWhite);
  void DrawTextBox(
      const std::string& font_name, int font_size,
      const std::string& text, const SDL_Rect& rect, const Point& dir,
      const SDL_Color fg_color=kWhite, const SDL_Color bg_color=kBlack);

 private:
  // The class has ownership of the loaded font.
  font::Font* LoadFont(const std::string& font_name, int font_size);

  FT_Library library_;
  SDL_Surface* target_;

  std::map<std::pair<std::string,int> ,font::Font*> fonts_by_id_;
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_TEXT_RENDERER_H__
