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

  // The caller takes ownership of the SDL_Texture in the returned Text.
  // This method does not take a color argument because the text color can be
  // set when rendering the text by calling SDL_SetTextureColorMod.
  Text DrawText(const std::string& font_name, int font_size,
                const std::string& text);

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
