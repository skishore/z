#include <algorithm>
#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "debug.h"
#include "TextRenderer.h"

using std::max;
using std::min;
using std::string;

namespace skishore {

using font::Font;

namespace font {
namespace {

static const int kDPI = 72;
static const int kScale = 64;

/*  See http://www.microsoft.com/typography/otspec/name.htm
  for a list of some possible platform-encoding pairs.
  We're interested in 0-3 aka 3-1 - UCS-2.
  Otherwise, fail. If a font has some unicode map, but lacks
  UCS-2 - it is a broken or irrelevant font. What exactly
  Freetype will select on face load (it promises most wide
  unicode, and if that will be slower that UCS-2 - left as
  an excercise to check. */
int ForceUCS2Charmap(FT_Face face) {
  for(int i = 0; i < face->num_charmaps; i++) {
    if (((face->charmaps[i]->platform_id == 0) &&
         (face->charmaps[i]->encoding_id == 3)) ||
        ((face->charmaps[i]->platform_id == 3) &&
         (face->charmaps[i]->encoding_id == 1))) {
      return FT_Set_Charmap(face, face->charmaps[i]);
    }
  }
  return -1;
}

// Point the compiler towards the expected branch of each if.
#ifndef unlikely
#define unlikely
#endif  // unlikely

struct SizerContext {
  int min_span_x;
  int max_span_x;
  int min_y;
  int max_y;
};

// Spanner that records the glyph size instead of writing it to the surface.
void Sizer(int y, int count, const FT_Span* spans, void* ctx) {
  SizerContext* context = (SizerContext*)ctx;
  context->min_y = min(y, context->min_y);
  context->max_y = max(y, context->max_y);
  for (int i = 0 ; i < count; i++) {
    context->min_span_x = min((int)spans[i].x, context->min_span_x);
    context->max_span_x = max(spans[i].x + spans[i].len, context->max_span_x);
  }
}

struct RendererContext {
  RendererContext(const SDL_Surface& surface)
      : pixels((uint32_t*)surface.pixels), width(surface.w), height(surface.h),
        pitch(surface.pitch), rshift(surface.format->Rshift),
        gshift(surface.format->Gshift), bshift(surface.format->Bshift) {}

  // Pixels stores a pointer to the surface's top-left pixel.
  uint32_t* pixels;
  int width;
  int height;
  uint32_t pitch;

  // Data about the 32-bit format of the surface's pixels.
  uint32_t rshift;
  uint32_t gshift;
  uint32_t bshift;

  // The current glyph's origin in surface coordinates.
  int gx;
  int gy;
};

// Render function that simply writes the glyph out to the surface.
// This method does not blend the glyph with the surface's current contents.
struct Overwrite {
  static inline void Render(uint32_t* target, uint32_t color) {
    *target = color;
  }
};

// Render function that max-blends the glyph on to the surface.
struct Blend {
  static inline void Render(uint32_t* target, uint32_t color) {
    *target |= color;
  }
};

template <typename T>
void Renderer(int y, int count, const FT_Span* spans, void* ctx) {
  RendererContext* context = (RendererContext*)ctx;
  y = context->gy - y;
  if (unlikely(y < 0 || y >= context->height)) {
    return;
  }
  uint32_t* scanline = context->pixels + y*((int)context->pitch/4);
  for (int i = 0; i < count; i++) {
    int x = context->gx + spans[i].x;
    if (unlikely(x < 0)) {
      break;
    }
    uint32_t* start = scanline + x;
    uint32_t color =
      (spans[i].coverage << context->rshift) |
      (spans[i].coverage << context->gshift) |
      (spans[i].coverage << context->bshift);

    for (int j = 0; j < spans[i].len; j++) {
      if (unlikely(x + j >= context->width)) {
        break;
      }
      T::Render(start + j, color);
    }
  }
}

void HLine(SDL_Surface* surface, int min_x, int max_x, int y, uint32_t color) {
  if (unlikely(y < 0 || y >= surface->h)) {
    return;
  }
  min_x = max(min_x, 0);
  max_x = min(max_x, surface->w - 1);
  uint32_t* pix = (uint32_t*)surface->pixels + (y*surface->pitch)/4 + min_x;
  uint32_t* end = (uint32_t*)surface->pixels + (y*surface->pitch)/4 + max_x;
  while (pix - 1 < end) {
    *pix = color;
    pix += 1;
  }
}

void VLine(SDL_Surface* surface, int x, int min_y, int max_y, uint32_t color) {
  if (unlikely(x < 0 || x >= surface->w)) {
    return;
  }
  min_y = max(min_y, 0);
  max_y = min(max_y, surface->h - 1);
  uint32_t* pix = (uint32_t*)surface->pixels + (min_y*surface->pitch)/4 + x;
  uint32_t* end = (uint32_t*)surface->pixels + (max_y*surface->pitch)/4 + x;
  while (pix - surface->pitch/4 < end) {
    *pix = color;
    pix += surface->pitch/4;
  }
}

hb_script_t GetHarfbuzzBufferScript(hb_buffer_t* buffer) {
  // Sets a best-guess value for the script of the given buffer.
  hb_unicode_funcs_t* unicode_funcs = hb_buffer_get_unicode_funcs(buffer);
  if (unicode_funcs == nullptr) {
    return HB_SCRIPT_UNKNOWN;
  }
  unsigned int num_glyphs;
  hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &num_glyphs);
  for (int i = 0; i < num_glyphs; i++) {
    hb_script_t script = hb_unicode_script(
        unicode_funcs, glyph_info[i].codepoint);
    if (script != HB_SCRIPT_COMMON && script != HB_SCRIPT_INHERITED &&
        script != HB_SCRIPT_UNKNOWN) {
      return script;
    }
  }
  return HB_SCRIPT_UNKNOWN;
}

}  // namespace

class Font {
 public:
  Font(const string& font_name, int font_size, FT_Library library);
  ~Font();

  void PrepareToRender(const string& text, Point* size, Point* baseline);
  void Render(const Point& position, const Point& size,
              const Point& baseline, SDL_Surface* target, bool blend=true);

 private:
  int font_size_;
  FT_Library library_;
  FT_Face face_;
  FT_Raster_Params renderer_;
  hb_font_t* font_;
  hb_buffer_t* buffer_;
};

Font::Font(const string& font_name, int font_size, FT_Library library)
    : font_size_(font_size), library_(library) {
  ASSERT(!FT_New_Face(library_, font_name.c_str(), 0, &face_),
         "Failed to load " << font_name);
  ASSERT(!FT_Set_Char_Size(face_, 0, kScale*font_size_, kDPI, kDPI),
         "Failed to set font size for " << font_name);
  ASSERT(!ForceUCS2Charmap(face_), "Failed to set charmap for " << font_name);
  font_ = hb_ft_font_create(face_, nullptr);
  buffer_ = hb_buffer_create();

  renderer_.target = nullptr;
  renderer_.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
  renderer_.black_spans = nullptr;
  renderer_.bit_set = nullptr;
  renderer_.bit_test = nullptr;
}

void Font::PrepareToRender(const string& text, Point* size, Point* baseline) {
  hb_buffer_clear_contents(buffer_);
  hb_buffer_set_direction(buffer_, HB_DIRECTION_LTR);
  hb_buffer_set_language(buffer_, hb_language_from_string("", 0));
  hb_buffer_add_utf8(buffer_, text.c_str(), text.size(), 0, text.size());
  hb_buffer_set_script(buffer_, GetHarfbuzzBufferScript(buffer_));
  hb_shape(font_, buffer_, nullptr, 0);

  unsigned int glyph_count;
  hb_glyph_info_t* glyph_info =
      hb_buffer_get_glyph_infos(buffer_, &glyph_count);
  hb_glyph_position_t* glyph_pos =
      hb_buffer_get_glyph_positions(buffer_, &glyph_count);

  SizerContext context;
  renderer_.user = &context;
  renderer_.gray_spans = Sizer;

  Point min_b(INT_MAX, INT_MAX);
  Point max_b(INT_MIN, INT_MIN);
  Point offset;

  for (int i = 0; i < glyph_count; i++) {
    ASSERT(!FT_Load_Glyph(face_, glyph_info[i].codepoint, 0),
           "Failed to load glyph: " << glyph_info[i].codepoint);
    ASSERT(face_->glyph->format == FT_GLYPH_FORMAT_OUTLINE,
           "Got unexpected glyph format: " << (char*)&face_->glyph->format);
    // Compute the glyph's x and y position in CHARACTER coordinates.
    // Note that in character coordinates, +y is UP.
    int gx = offset.x + glyph_pos[i].x_offset/kScale;
    int gy = offset.y + glyph_pos[i].y_offset/kScale;
    context.min_span_x = INT_MAX;
    context.max_span_x = INT_MIN;
    context.min_y = INT_MAX;
    context.max_y = INT_MIN;
    ASSERT(!FT_Outline_Render(library_, &face_->glyph->outline, &renderer_),
           "Failed to render " << glyph_info[i].codepoint);
    if (context.min_span_x != INT_MAX) {
      min_b.x = min(context.min_span_x + gx, min_b.x);
      max_b.x = max(context.max_span_x + gx, max_b.x);
      min_b.y = min(context.min_y + gy, min_b.y);
      max_b.y = max(context.max_y + gy, max_b.y);
    } else {
      min_b.x = min(gx, min_b.x);
      max_b.x = max(gx, max_b.x);
      min_b.y = min(gy, min_b.y);
      max_b.y = max(gy, max_b.y);
    }
    offset.x += glyph_pos[i].x_advance/kScale;
    offset.y += glyph_pos[i].y_advance/kScale;
  }
  // TODO(skishore): This extra extension of the bounding box may be unneeded.
  min_b.x = min(offset.x, min_b.x);
  max_b.x = max(offset.x, max_b.x);
  min_b.y = min(offset.y, min_b.y);
  max_b.y = max(offset.y, max_b.y);

  *size = max_b - min_b;
  baseline->x = -min_b.x;
  baseline->y = max_b.y;
}

void Font::Render(const Point& position, const Point& size,
                  const Point& baseline, SDL_Surface* surface, bool blend) {
  int x = position.x;
  int y = position.y;

  SDL_LockSurface(surface);

  HLine(surface, x + baseline.x, x + size.x, y + baseline.y, 0x0000ff00);
  HLine(surface, x, x + size.x, y, 0x00ff0000);
  HLine(surface, x, x + size.x, y + size.y, 0x00ff0000);
  VLine(surface, x, y, y + size.y, 0x00ff0000);
  VLine(surface, x + size.x, y, y + size.y, 0x00ff0000);

  x += baseline.x;
  y += baseline.y;

  unsigned int glyph_count;
  hb_glyph_info_t* glyph_info =
      hb_buffer_get_glyph_infos(buffer_, &glyph_count);
  hb_glyph_position_t* glyph_pos =
      hb_buffer_get_glyph_positions(buffer_, &glyph_count);

  RendererContext context(*surface);
  renderer_.user = &context;
  renderer_.gray_spans = (blend ? Renderer<Blend> : Renderer<Overwrite>);

  for (int i = 0; i < glyph_count; i++) {
    ASSERT(!FT_Load_Glyph(face_, glyph_info[i].codepoint, 0),
           "Failed to load glyph: " << glyph_info[i].codepoint);
    ASSERT(face_->glyph->format == FT_GLYPH_FORMAT_OUTLINE,
           "Got unexpected glyph format: " << (char*)&face_->glyph->format);
    // Compute the glyph's x and y position in WINDOW coordinates.
    // Note that in character coordinates, +y is DOWN.
    context.gx = x + glyph_pos[i].x_offset/kScale;
    context.gy = y - glyph_pos[i].y_offset/kScale;
    ASSERT(!FT_Outline_Render(library_, &face_->glyph->outline, &renderer_),
           "Failed to render " << glyph_info[i].codepoint);
    x += glyph_pos[i].x_advance/kScale;
    y -= glyph_pos[i].y_advance/kScale;
  }

  SDL_UnlockSurface(surface);
}

Font::~Font() {
  hb_buffer_destroy(buffer_);
  hb_font_destroy(font_);
  FT_Done_Face(face_);
}

}  // namespace font

TextRenderer::TextRenderer(const SDL_Rect& bounds, SDL_Surface* target)
    : target_(target) {
  ASSERT(!FT_Init_FreeType(&library_), "Failed to initialize freetype!");
}

TextRenderer::~TextRenderer() {
  for (auto& pair : fonts_by_size_) {
    delete pair.second;
  }
  FT_Done_FreeType(library_);
}

void TextRenderer::DrawText(int font_size, const Point& position,
                            const string& text, const SDL_Color color) {
  if (text.size() == 0) {
    return;
  }
  Font* font = LoadFont(font_size);
  Point size, baseline;
  font->PrepareToRender(text, &size, &baseline);
  font->Render(position, size, baseline, target_);
}

void TextRenderer::DrawTextBox(
    int font_size, const SDL_Rect& rect, const string& text,
    const SDL_Color fg_color, const SDL_Color bg_color) {
  if (text.size() == 0) {
    return;
  }
  Font* font = LoadFont(font_size);
  Point size, baseline;
  font->PrepareToRender(text, &size, &baseline);
  Point position(rect.x + rect.w, rect.y - rect.h);
  font->Render(position, size, baseline, target_);
}

Font* TextRenderer::LoadFont(int font_size) {
  if (fonts_by_size_.count(font_size) == 0) {
    DEBUG("Loading font with size " << font_size);
    Font* font = new Font("fonts/default_font.ttf", font_size, library_);
    fonts_by_size_[font_size] = font;
  }
  return fonts_by_size_[font_size];
}

} // namespace skishore
