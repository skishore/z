#ifndef __BABEL_RENDER_TEXT_RENDERER_H__
#define __BABEL_RENDER_TEXT_RENDERER_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "base/point.h"

namespace babel {
namespace render {

static const uint32_t kBlack = 0x00000000;
static const uint32_t kWhite = 0x00ffffff;

namespace font {
class Font;
}  // namespace font

struct Text {
  Point size;
  Point baseline;
  SDL_Texture* texture;
};

class TextRenderer {
 public:
  TextRenderer(SDL_Renderer* renderer);
  ~TextRenderer();

  // The caller takes ownership of the SDL_Surface in the returned Text.
  Text DrawText(const std::string& font_name, int font_size,
                const std::string& text, uint32_t color=kWhite);

 private:
  // This class owns the loaded font.
  font::Font* LoadFont(const std::string& font_name, int font_size);

  FT_Library library_;
  std::map<std::pair<std::string,int> ,font::Font*> fonts_by_id_;
  SDL_Renderer* renderer_;
};

} // namespace render
} // namespace babel

#endif  // __BABEL_RENDER_TEXT_RENDERER_H__
