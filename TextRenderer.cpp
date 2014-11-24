#include "debug.h"
#include "TextRenderer.h"

namespace skishore {

TextRenderer::TextRenderer(const SDL_Rect& bounds, SDL_Surface* target)
    : target_(target) { //bounds_(bounds), target_(target) {
  TTF_Init();
}

TextRenderer::~TextRenderer() {
  for (auto& pair : fonts_by_size_) {
    TTF_CloseFont(pair.second);
  }
  TTF_Quit();
}

void TextRenderer::DrawText(int font_size, const Point& position,
                            const std::string text, const SDL_Color color) {
  if (text.size() == 0) {
    return;
  }
  TTF_Font* font = LoadFont(font_size);
  SDL_Surface* surface(TTF_RenderUTF8_Solid(font, text.c_str(), color));
  ASSERT(surface != nullptr, TTF_GetError());
  SDL_Rect target{position.x, position.y, 0, 0};
  SDL_BlitSurface(surface, nullptr, target_, &target);
  SDL_FreeSurface(surface);
}

void TextRenderer::DrawTextBox(
    int font_size, const SDL_Rect& rect, const std::string text,
    const SDL_Color fg_color, const SDL_Color bg_color) {
  if (text.size() == 0) {
    return;
  }
  TTF_Font* font = LoadFont(font_size);
  SDL_Surface* surface(TTF_RenderUTF8_Solid(font, text.c_str(), fg_color));
  ASSERT(surface != nullptr, TTF_GetError());
  SDL_Rect target{rect.x + rect.w, rect.y - rect.h, 0, 0};
  SDL_BlitSurface(surface, nullptr, target_, &target);
  SDL_FreeSurface(surface);
}

TTF_Font* TextRenderer::LoadFont(int font_size) {
  if (fonts_by_size_.count(font_size) == 0) {
    DEBUG("Loading font with size " << font_size);
    TTF_Font* font = TTF_OpenFont("fonts/default_font.ttf", font_size);
    ASSERT(font != nullptr, TTF_GetError());
    fonts_by_size_[font_size] = font;
  }
  return fonts_by_size_[font_size];
}

} // namespace skishore
