#ifndef __SKISHORE_TEXT_RENDERER_H__
#define __SKISHORE_TEXT_RENDERER_H__

#include <map>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Point.h"

namespace skishore {

namespace {
static const SDL_Color kDefaultColor{255, 255, 255};
}  // namespace

class TextRenderer {
 public:
  TextRenderer(const SDL_Rect& bounds, SDL_Surface* target);
  ~TextRenderer();

  void DrawText(int font_size, const Point& position,
                const std::string text, const SDL_Color color=kDefaultColor);

 private:
  TTF_Font* LoadFont(int font_size);

  //const SDL_Rect& bounds_;
  SDL_Surface* target_;

  std::map<int,TTF_Font*> fonts_by_size_;
};

} // namespace skishore

#endif  // __SKISHORE_TEXT_RENDERER_H__
