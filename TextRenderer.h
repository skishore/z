#ifndef __SKISHORE_TEXT_RENDERER_H__
#define __SKISHORE_TEXT_RENDERER_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "constants.h"
#include "Point.h"

namespace skishore {

static const SDL_Color kBlack{0, 0, 0};
static const SDL_Color kYellow{255, 0, 0};
static const SDL_Color kWhite{255, 255, 255};

namespace font {
class Font;
}  // namespace font

class TextRenderer {
 public:
  TextRenderer(const SDL_Rect& bounds, SDL_Surface* target);
  ~TextRenderer();

  void DrawText(int font_size, const std::string& text,
                const Point& position, const SDL_Color color=kYellow);
  void DrawTextBox(
      int font_size, Direction dir, const std::string& text,
      const SDL_Rect& rect, const SDL_Color fg_color=kWhite,
      const SDL_Color bg_color=kBlack);

 private:
  // The class has ownership of the loaded font.
  font::Font* LoadFont(int font_size);

  FT_Library library_;
  SDL_Surface* target_;

  std::map<int,font::Font*> fonts_by_size_;
};

} // namespace skishore

#endif  // __SKISHORE_TEXT_RENDERER_H__
