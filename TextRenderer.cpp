#include <algorithm>
#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "debug.h"
#include "SDL_prims.h"
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

// Extract the unicode map based on hacky platform/encoding ids.
// UCS-2, the Unicode character map, is 0-3 or 3-1.
// See http://www.microsoft.com/typography/otspec/name.htm for details.
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
  int x = position.x + baseline.x;
  int y = position.y + baseline.y;

  unsigned int glyph_count;
  hb_glyph_info_t* glyph_info =
      hb_buffer_get_glyph_infos(buffer_, &glyph_count);
  hb_glyph_position_t* glyph_pos =
      hb_buffer_get_glyph_positions(buffer_, &glyph_count);

  RendererContext context(*surface);
  renderer_.user = &context;
  renderer_.gray_spans = (blend ? Renderer<Blend> : Renderer<Overwrite>);

  SDL_LockSurface(surface);

  #ifdef EMSCRIPTEN
  // TODO(skishore): Why does auto-hinting crash emscripten?
  const Uint32 load_flags = 0;
  #else  // EMSCRIPTEN
  const Uint32 load_flags = FT_LOAD_FORCE_AUTOHINT;
  #endif // EMSCRIPTEN
  for (int i = 0; i < glyph_count; i++) {
    ASSERT(!FT_Load_Glyph(face_, glyph_info[i].codepoint, load_flags),
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

namespace {

// In text boxes, ASCII text is drawn smaller than arbitrary Unicode text.
int CorrectFontSize(int font_size, const string& text) {
  for (char ch : text) {
    if ((uint8_t)ch >= 1 << 7) {
      return font_size;
    }
  }
  return 3*font_size/4;
}

SDL_Point* GetTextPolygon(int font_size, Direction dir, const SDL_Rect& rect,
                          const Point& size, Point* position) {
  const int kWedge = font_size/3;
  const Point kPadding(font_size/3, font_size/3);
  SDL_Point* polygon = new SDL_Point[7];
  if (dir == Direction::UP) {
    polygon[0].x = rect.x + rect.w/2;
    polygon[0].y = rect.y - kWedge/2;
    polygon[1].x = polygon[0].x - kWedge;
    polygon[1].y = polygon[0].y - kWedge;
    polygon[2].x = polygon[0].x - size.x/2 - kPadding.x;
    polygon[2].y = polygon[1].y;
    polygon[3].x = polygon[2].x;
    polygon[3].y = polygon[2].y - size.y - 2*kPadding.y;
    polygon[4].x = polygon[3].x + size.x + 2*kPadding.x;
    polygon[4].y = polygon[3].y;
    polygon[5].x = polygon[4].x;
    polygon[5].y = polygon[1].y;
    polygon[6].x = polygon[0].x + kWedge;
    polygon[6].y = polygon[1].y;
  } else if (dir == Direction::LEFT || dir == Direction::RIGHT) {
    const int sign = (dir == Direction::RIGHT ? 1 : -1);
    polygon[0].x = rect.x + (sign + 1)*rect.w/2;
    polygon[0].y = rect.y + rect.h/2 - 1;
    polygon[1].x = polygon[0].x + sign*kWedge;
    polygon[1].y = polygon[0].y - kWedge;
    polygon[2].x = polygon[1].x;
    polygon[2].y = polygon[0].y - size.y/2 - kPadding.y;
    polygon[3].x = polygon[2].x + sign*(size.x + 2*kPadding.x);
    polygon[3].y = polygon[2].y;
    polygon[4].x = polygon[3].x;
    polygon[4].y = polygon[3].y + size.y + 2*kPadding.y;
    polygon[5].x = polygon[1].x;
    polygon[5].y = polygon[4].y;
    polygon[6].x = polygon[1].x;
    polygon[6].y = polygon[0].y + kWedge;
  } else {
    ASSERT(false, "Unexpected text direction: " << dir);
  }
  int top_left = (dir == Direction::RIGHT ? 2 : 3);
  *position = Point(polygon[top_left].x, polygon[top_left].y) + kPadding;
  return polygon;
}

}  // namespace

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

void TextRenderer::DrawText(int font_size, const string& text,
                            const Point& position, const SDL_Color color) {
  if (text.size() == 0) {
    return;
  }
  Font* font = LoadFont(font_size);
  Point size, baseline;
  font->PrepareToRender(text, &size, &baseline);
  Point adjusted_position = position;
  adjusted_position.y += font_size - baseline.y;
  font->Render(adjusted_position, size, baseline, target_);
}

void TextRenderer::DrawTextBox(
    int font_size, Direction dir, const string& text, const SDL_Rect& rect,
    const SDL_Color fg_color, const SDL_Color bg_color) {
  if (text.size() == 0) {
    return;
  }
  font_size = CorrectFontSize(font_size, text);
  Font* font = LoadFont(font_size);
  Point size, baseline, position;
  font->PrepareToRender(text, &size, &baseline);
  std::unique_ptr<SDL_Point[]> polygon(
      GetTextPolygon(font_size, dir, rect, size, &position));
  // TODO(skishore): Convert the fg_ and bg_ colors to Uint32 here.
  SDL_FillPolygon(target_, polygon.get(), 7, 0x00002266);
  SDL_DrawPolygon(target_, polygon.get(), 7, 0x00ffffff);
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
