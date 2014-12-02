#include <codecvt>
#include <locale>

#include <hb.h>
#include <hb-ft.h>

#include "debug.h"
#include "TextRenderer.h"

using std::string;
using std::u16string;

namespace skishore {

namespace {
std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t> kConvert;
u16string ConvertToUTF16(const string& utf8) {
  return kConvert.from_bytes(utf8);
}
}  // namespace

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
                            const string& text, const SDL_Color color) {
  if (text.size() == 0) {
    return;
  }
  SDL_Rect size;
  SDL_Surface* surface = RenderTextSolid(font_size, text, color, &size);
  SDL_Rect target{position.x, position.y, 0, 0};
  SDL_BlitSurface(surface, nullptr, target_, &target);
  SDL_FreeSurface(surface);
}

void TextRenderer::DrawTextBox(
    int font_size, const SDL_Rect& rect, const string& text,
    const SDL_Color fg_color, const SDL_Color bg_color) {
  if (text.size() == 0) {
    return;
  }
  SDL_Rect size;
  SDL_Surface* surface = RenderTextSolid(font_size, text, fg_color, &size);
  SDL_Rect target{rect.x + rect.w, rect.y - rect.h, 0, 0};
  SDL_BlitSurface(surface, nullptr, target_, &target);
  SDL_FreeSurface(surface);
}

SDL_Surface* TextRenderer::RenderTextSolid(int font_size, const string& text,
                                           SDL_Color color, SDL_Rect* size) {
  TTF_Font* font = LoadFont(font_size);
  u16string u16text = ConvertToUTF16(text);
  SDL_Surface* surface = TTF_RenderUNICODE_Solid(
      font, (const Uint16*)u16text.c_str(), color);
  ASSERT(surface != nullptr, TTF_GetError());
  return surface;
}

namespace {
/*  See http://www.microsoft.com/typography/otspec/name.htm
  for a list of some possible platform-encoding pairs.
  We're interested in 0-3 aka 3-1 - UCS-2.
  Otherwise, fail. If a font has some unicode map, but lacks
  UCS-2 - it is a broken or irrelevant font. What exactly
  Freetype will select on face load (it promises most wide
  unicode, and if that will be slower that UCS-2 - left as
  an excercise to check. */
int force_ucs2_charmap(FT_Face ftf) {
  for(int i = 0; i < ftf->num_charmaps; i++) {
    if (((ftf->charmaps[i]->platform_id == 0) &&
         (ftf->charmaps[i]->encoding_id == 3)) ||
        ((ftf->charmaps[i]->platform_id == 3) &&
         (ftf->charmaps[i]->encoding_id == 1))) {
      return FT_Set_Charmap(ftf, ftf->charmaps[i]);
    }
  }
  return -1;
}
}  // namespace

TTF_Font* TextRenderer::LoadFont(int font_size) {
  if (fonts_by_size_.count(font_size) == 0) {
    DEBUG("Loading font with size " << font_size);
    TTF_Font* font = TTF_OpenFont("fonts/default_font.ttf", font_size);

    static const int kDPI = 72;
    static const int kFontSize = 48;
    static const int kScale = 64;
    FT_Library ft_library;
    ASSERT(!FT_Init_FreeType(&ft_library), "Failed to initialize freetype!");
    FT_Face ft_face;
    ASSERT(!FT_New_Face(ft_library, "fonts/default_font.ttf", 0, &ft_face), "Failed to load font!");
    ASSERT(!FT_Set_Char_Size(ft_face, 0, kFontSize*kScale, kDPI, kDPI), "Failed to set font size!");
    ASSERT(!force_ucs2_charmap(ft_face), "Failed to set charmap!");
    hb_font_t* hb_ft_font = hb_ft_font_create(ft_face, nullptr);
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
    hb_buffer_set_script(buf, HB_SCRIPT_DEVANAGARI);
    hb_buffer_set_language(buf, hb_language_from_string("", 0));
    const string kTestText = "\u0924\u094D\u0928";
    int length = kTestText.size();
    hb_buffer_add_utf8(buf, kTestText.c_str(), length, 0, length);
    hb_shape(hb_ft_font, buf, NULL, 0);
    unsigned int glyph_count;
    //hb_glyph_info_t* glyph_info =
        hb_buffer_get_glyph_infos(buf, &glyph_count);
    //hb_glyph_position_t* glyph_pos =
        hb_buffer_get_glyph_positions(buf, &glyph_count);
    ASSERT(glyph_count == 2, "harfbuzz smoke test failed!");
    DEBUG("String w/ 3 Unicode code points shaped into " << glyph_count << " glyphs");
    DEBUG("harfbuzz smoke test passed!");

    ASSERT(font != nullptr, TTF_GetError());
    fonts_by_size_[font_size] = font;
  }
  return fonts_by_size_[font_size];
}

} // namespace skishore
