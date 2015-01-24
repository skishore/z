#include <algorithm>
#include <hb.h>
#include <hb-ft.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H

#include "base/debug.h"
#include "render/SDL_prims.h"
#include "render/TextRenderer.h"

using std::max;
using std::min;
using std::pair;
using std::string;

namespace babel {
namespace render {

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

void LoadFace(const string& font_name, int font_size,
              FT_Library library, FT_Face* face) {
  ASSERT(!FT_New_Face(library, font_name.c_str(), 0, face),
         "Failed to load " << font_name);
  ASSERT(!FT_Set_Char_Size(*face, 0, kScale*font_size, kDPI, kDPI),
         "Failed to set font size for " << font_name);
  ASSERT(!ForceUCS2Charmap(*face), "Failed to set charmap for " << font_name);
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
  RendererContext(const SDL_Surface& surface, const SDL_Color c)
      : pixels((uint32_t*)surface.pixels), width(surface.w), height(surface.h),
        pitch(surface.pitch), rshift(surface.format->Rshift),
        gshift(surface.format->Gshift), bshift(surface.format->Bshift),
        color(c) {
    invert = color.r + color.g + color.b == 0;
    if (invert) {
      color.r = ~color.r;
      color.g = ~color.g;
      color.b = ~color.b;
    }
  }

  // Used to render on a light background.
  bool invert;

  // Pixels stores a pointer to the surface's top-left pixel.
  uint32_t* pixels;
  int width;
  int height;
  uint32_t pitch;

  // Data about the 32-bit format of the surface's pixels.
  uint32_t rshift;
  uint32_t gshift;
  uint32_t bshift;
  SDL_Color color;

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

static const int kMaxChar = (1 << 8) - 1;

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
      ((int)(context->color.r*spans[i].coverage/kMaxChar) << context->rshift) |
      ((int)(context->color.g*spans[i].coverage/kMaxChar) << context->gshift) |
      ((int)(context->color.b*spans[i].coverage/kMaxChar) << context->bshift);

    for (int j = 0; j < spans[i].len; j++) {
      if (unlikely(x + j >= context->width)) {
        break;
      }
      uint32_t* target = start + j;
      if (context->invert) {
        *target = ~*target;
        T::Render(target, color);
        *target = ~*target;
      } else {
        T::Render(target, color);
      }
    }
  }
}

inline hb_script_t GetGlyphScript(const hb_glyph_info_t& glyph_info,
                                  hb_unicode_funcs_t* unicode_funcs) {
  hb_script_t script = hb_unicode_script(unicode_funcs, glyph_info.codepoint);
  if (script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED) {
    return HB_SCRIPT_UNKNOWN;
  }
  return script;
}

}  // namespace

class Font {
 public:
  Font(const string& font_name, int font_size, FT_Library library);
  ~Font();

  void PrepareToRender(const string& text, Point* size, Point* baseline);
  void Render(const Point& position, const Point& size, const Point& baseline,
              const SDL_Color color, SDL_Surface* target, bool blend=true);

 private:
  inline bool UseBigGlyph(const hb_glyph_info_t& glyph_info);

  FT_Library library_;
  FT_Face face_;
  FT_Face big_face_;
  FT_Raster_Params renderer_;
  hb_font_t* font_;
  hb_buffer_t* buffer_;
  hb_unicode_funcs_t* unicode_funcs_;
};

Font::Font(const string& font_name, int font_size, FT_Library library)
    : library_(library) {
  LoadFace(font_name, font_size, library, &face_);
  LoadFace(font_name, 1.15*font_size, library, &big_face_);
  font_ = hb_ft_font_create(face_, nullptr);
  buffer_ = hb_buffer_create();
  unicode_funcs_ = hb_buffer_get_unicode_funcs(buffer_);
  ASSERT(unicode_funcs_ != nullptr, "Failed to get HarfBuzz unicode_funcs!");

  renderer_.target = nullptr;
  renderer_.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
  renderer_.black_spans = nullptr;
  renderer_.bit_set = nullptr;
  renderer_.bit_test = nullptr;
}

Font::~Font() {
  hb_buffer_destroy(buffer_);
  hb_font_destroy(font_);
  FT_Done_Face(big_face_);
  FT_Done_Face(face_);
}

void Font::PrepareToRender(const string& text, Point* size, Point* baseline) {
  DEBUG(text);
  hb_buffer_clear_contents(buffer_);
  hb_buffer_set_direction(buffer_, HB_DIRECTION_LTR);
  hb_buffer_set_language(buffer_, hb_language_from_string("", 0));
  hb_buffer_add_utf8(buffer_, text.c_str(), text.size(), 0, text.size());
  hb_buffer_set_script(buffer_, HB_SCRIPT_DEVANAGARI);
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
    FT_Face face = (UseBigGlyph(glyph_info[i]) ? big_face_ : face_);
    ASSERT(!FT_Load_Glyph(face, glyph_info[i].codepoint, 0),
           "Failed to load glyph: " << glyph_info[i].codepoint);
    ASSERT(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE,
           "Got unexpected glyph format: " << (char*)&face->glyph->format);

    // Compute the glyph's x and y position in CHARACTER coordinates.
    // Note that in character coordinates, +y is UP.
    int gx = offset.x + glyph_pos[i].x_offset/kScale;
    int gy = offset.y + glyph_pos[i].y_offset/kScale;
    context.min_span_x = INT_MAX;
    context.max_span_x = INT_MIN;
    context.min_y = INT_MAX;
    context.max_y = INT_MIN;
    ASSERT(!FT_Outline_Render(library_, &face->glyph->outline, &renderer_),
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

  *size = max_b - min_b - 1;
  baseline->x = -min_b.x;
  baseline->y = max_b.y;
}

void Font::Render(
    const Point& position, const Point& size, const Point& baseline,
    const SDL_Color color, SDL_Surface* surface, bool blend) {
  int x = position.x + baseline.x;
  int y = position.y + baseline.y;

  unsigned int glyph_count;
  hb_glyph_info_t* glyph_info =
      hb_buffer_get_glyph_infos(buffer_, &glyph_count);
  hb_glyph_position_t* glyph_pos =
      hb_buffer_get_glyph_positions(buffer_, &glyph_count);

  RendererContext context(*surface, color);
  renderer_.user = &context;
  renderer_.gray_spans = (blend ? Renderer<Blend> : Renderer<Overwrite>);

  SDL_LockSurface(surface);

  #ifdef EMSCRIPTEN
  // TODO(babel): Why does auto-hinting crash emscripten?
  const Uint32 load_flags = 0;
  #else  // EMSCRIPTEN
  const Uint32 load_flags = FT_LOAD_FORCE_AUTOHINT;
  #endif // EMSCRIPTEN
  for (int i = 0; i < glyph_count; i++) {
    hb_script_t script = GetGlyphScript(glyph_info[i], unicode_funcs_);
    DEBUG((glyph_info[i].codepoint));
    DEBUG((char)(script >> 24) << (char)(script >> 16) << (char)(script >> 8) << (char)(script));
    FT_Face face = (UseBigGlyph(glyph_info[i]) ? big_face_ : face_);
    ASSERT(!FT_Load_Glyph(face, glyph_info[i].codepoint, load_flags),
           "Failed to load glyph: " << glyph_info[i].codepoint);
    ASSERT(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE,
           "Got unexpected glyph format: " << (char*)&face->glyph->format);

    // Compute the glyph's x and y position in WINDOW coordinates.
    // Note that in character coordinates, +y is DOWN.
    context.gx = x + glyph_pos[i].x_offset/kScale;
    context.gy = y - glyph_pos[i].y_offset/kScale;
    ASSERT(!FT_Outline_Render(library_, &face->glyph->outline, &renderer_),
           "Failed to render " << glyph_info[i].codepoint);
    x += glyph_pos[i].x_advance/kScale;
    y -= glyph_pos[i].y_advance/kScale;
  }

  SDL_UnlockSurface(surface);
}

bool Font::UseBigGlyph(const hb_glyph_info_t& glyph_info) {
  // TODO(skishore): This check is a complete hack.
  // We should split the string into English and Hindi components and render
  // each component separately.
  return glyph_info.codepoint > (1 << 8) && glyph_info.codepoint < 3065;
}

}  // namespace font

namespace {

SDL_Point* GetTextPolygon(
    int font_size, const SDL_Rect& rect, const Point& dir,
    const Point& size, Point* position, int* vertices) {
  ASSERT(!dir.zero(), "Text box direction was empty!");
  const int kWedge = font_size/3;
  // Handle the cardinal direction cases first. For these cases, we return
  // a polygon with 7 vertices: 3 for the wedge and 4 for the actual text.
  if (dir.x == 0 || dir.y == 0) {
    int top_left_index;
    SDL_Point* polygon = new SDL_Point[7];
    if (dir.y == 0) {
      top_left_index = (dir.x > 0 ? 2 : 3);
      polygon[0].x = rect.x + (dir.x + 1)*rect.w/2;
      polygon[0].y = rect.y + rect.h/2;
      polygon[1].x = polygon[0].x + dir.x*kWedge;
      polygon[1].y = polygon[0].y - kWedge;
      polygon[2].x = polygon[1].x;
      polygon[2].y = rect.y;
      polygon[3].x = polygon[2].x + dir.x*size.x;
      polygon[3].y = polygon[2].y;
      polygon[4].x = polygon[3].x;
      polygon[4].y = rect.y + rect.h;
      polygon[5].x = polygon[1].x;
      polygon[5].y = polygon[4].y;
      polygon[6].x = polygon[1].x;
      polygon[6].y = polygon[0].y + kWedge;
    } else {
      top_left_index = (dir.y > 0 ? 2 : 3);
      polygon[0].x = rect.x + rect.w/2;
      polygon[0].y = rect.y + (dir.y + 1)*rect.h/2;
      polygon[1].x = polygon[0].x - kWedge;
      polygon[1].y = polygon[0].y + dir.y*kWedge;
      polygon[2].x = polygon[0].x - size.x/2;
      polygon[2].y = polygon[1].y;
      polygon[3].x = polygon[2].x;
      polygon[3].y = polygon[2].y + dir.y*rect.h;
      polygon[4].x = polygon[3].x + size.x;
      polygon[4].y = polygon[3].y;
      polygon[5].x = polygon[4].x;
      polygon[5].y = polygon[1].y;
      polygon[6].x = polygon[0].x + kWedge;
      polygon[6].y = polygon[1].y;
    }
    *position = Point(polygon[top_left_index].x, polygon[top_left_index].y);
    *vertices = 7;
    return polygon;
  }
  // Handle the diagonal cases. These cases are identical, up to sign.
  SDL_Point* polygon = new SDL_Point[5];
  polygon[0].x = rect.x + rect.w/2 - dir.x*rect.w/6;
  polygon[0].y = rect.y + (1.2*dir.y + 1)*rect.h/2;
  polygon[1].x = polygon[0].x + 2*dir.x*kWedge;
  polygon[1].y = polygon[0].y + dir.y*kWedge;
  polygon[2].x = polygon[0].x + dir.x*size.x;
  polygon[2].y = polygon[1].y;
  polygon[3].x = polygon[2].x;
  polygon[3].y = polygon[2].y + dir.y*rect.h;
  polygon[4].x = polygon[0].x;
  polygon[4].y = polygon[3].y;
  position->x = (dir.x > 0 ? polygon[0].x : polygon[2].x);
  position->y = (dir.y > 0 ? polygon[2].y : polygon[3].y);
  *vertices = 5;
  return polygon;
}

Uint32 ConvertColor(SDL_Color color, float lightness) {
  unsigned char r = (1 - lightness)*color.r + lightness*255;
  unsigned char g = (1 - lightness)*color.g + lightness*255;
  unsigned char b = (1 - lightness)*color.b + lightness*255;
  return (r << 16) + (g << 8) + b;
}

}  // namespace

TextRenderer::TextRenderer(const SDL_Rect& bounds, SDL_Surface* target)
    : target_(target) {
  ASSERT(!FT_Init_FreeType(&library_), "Failed to initialize freetype!");
}

TextRenderer::~TextRenderer() {
  for (auto& pair : fonts_by_id_) {
    delete pair.second;
  }
  FT_Done_FreeType(library_);
}

void TextRenderer::DrawText(
    const string& font_name, int font_size,
    const string& text, const SDL_Rect& rect, const SDL_Color color) {
  if (text.size() == 0) {
    return;
  }
  Font* font = LoadFont(font_name, font_size);
  Point size, baseline;
  font->PrepareToRender(text, &size, &baseline);
  Point position(rect.x, rect.y + 3*rect.h/4 - baseline.y);
  font->Render(position, size, baseline, color, target_);
}

void TextRenderer::DrawTextBox(
    const string& font_name, int font_size,
    const string& text, const SDL_Rect& rect, const Point& dir,
    const SDL_Color fg_color, const SDL_Color bg_color) {
  if (text.size() == 0) {
    return;
  }
  Font* font = LoadFont(font_name, font_size);
  Point size, baseline, position;
  font->PrepareToRender(text, &size, &baseline);

  const int border = (dir.x == 0 ? 0 : rect.w/8);
  const int padding = rect.w/8;
  size.x += border + 2*padding;

  int vertices;
  std::unique_ptr<SDL_Point[]> polygon(
      GetTextPolygon(font_size, rect, dir, size, &position, &vertices));
  Uint32 background = ConvertColor(bg_color, 0);
  SDL_FillPolygon(target_, polygon.get(), vertices, background);

  size.x -= border;
  SDL_Rect text_rect{position.x, position.y + 1, size.x + 1, rect.h};
  if (dir.x > 0) {
    text_rect.x += border;
    position.x += border;
  }
  background = ConvertColor(bg_color, 0.75);
  SDL_FillRect(target_, &text_rect, background);

  position.x += padding;
  position.y += (rect.h - size.y + 1)/2;
  font->Render(position, size, baseline, fg_color, target_);
}

Font* TextRenderer::LoadFont(const string& font_name, int font_size) {
  pair<string,int> id{font_name, font_size};
  if (fonts_by_id_.count(id) == 0) {
    DEBUG("Loaded " << font_name << " in size " << font_size);
    Font* font = new Font("fonts/" + font_name, font_size, library_);
    fonts_by_id_[id] = font;
  }
  return fonts_by_id_[id];
}

} // namespace render
} // namespace babel
