#include <algorithm>
#include <vector>

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
using std::vector;

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
  RendererContext(const SDL_Surface& surface, uint32_t c)
      : pixels((uint32_t*)surface.pixels), width(surface.w), height(surface.h),
        pitch(surface.pitch), color(c) {}

  // Pixels stores a pointer to the surface's top-left pixel.
  uint32_t* pixels;
  int width;
  int height;
  uint32_t pitch;

  // TODO(skishore): Make this renderer work for non-ARGB surfaces by reading
  // and storing the color format here.
  uint32_t color;

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

template<typename T>
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
    // TODO(skishore): We really need to use the window format here.
    uint32_t color = (context->color << 8) | spans[i].coverage;
    uint32_t* start = scanline + x;

    for (int j = 0; j < spans[i].len; j++) {
      if (unlikely(x + j >= context->width)) {
        break;
      }
      T::Render(start + j, color);
    }
  }
}

inline hb_script_t GetScript(
    hb_codepoint_t codepoint, hb_unicode_funcs_t* unicode_funcs) {
  if (codepoint == ' ') {
    return HB_SCRIPT_UNKNOWN;
  } else if (codepoint < 1 << 7) {
    return HB_SCRIPT_LATIN;
  }
  return hb_unicode_script(unicode_funcs, codepoint);
}

hb_buffer_t* CreateBuffer() {
  hb_buffer_t* result = hb_buffer_create();
  hb_buffer_clear_contents(result);
  hb_buffer_set_direction(result, HB_DIRECTION_LTR);
  hb_buffer_set_language(result, hb_language_from_string("", 0));
  return result;
}

void SegmentBuffer(hb_unicode_funcs_t* unicode_funcs, hb_buffer_t* buffer,
                   vector<hb_buffer_t*>* segments,
                   vector<hb_script_t>* scripts) {
  for (auto& segment : *segments) {
    hb_buffer_destroy(segment);
  }
  segments->clear();
  scripts->clear();

  unsigned int num_glyphs;
  hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buffer, &num_glyphs);
  if (num_glyphs == 0) {
    return;
  }

  segments->push_back(CreateBuffer());
  hb_script_t segment_script = HB_SCRIPT_UNKNOWN;
  scripts->push_back(HB_SCRIPT_UNKNOWN);

  for (int i = 0; i < num_glyphs; i++) {
    hb_script_t script = GetScript(glyph_info[i].codepoint, unicode_funcs);
    if (script != HB_SCRIPT_UNKNOWN && script != segment_script) {
      if (segment_script == HB_SCRIPT_UNKNOWN) {
        (*scripts)[scripts->size() - 1] = script;
      } else {
        segments->push_back(CreateBuffer());
        scripts->push_back(script);
      }
      segment_script = script;
    }
    hb_buffer_add_codepoints((*segments)[segments->size() - 1],
                             &glyph_info[i].codepoint, 1, 0, 1);
  }
  ASSERT(scripts->size() == segments->size(), "Mismatch segment sizes!");
}

}  // namespace

class Font {
 public:
  Font(const string& font_name, int font_size, FT_Library library);
  ~Font();

  void PrepareToRender(const string& text, Point* size, Point* baseline);
  void Render(const Point& position, const Point& size, const Point& baseline,
              uint32_t color, SDL_Surface* target, bool blend=true);

 private:
  FT_Library library_;
  FT_Face face_;
  FT_Face big_face_;
  FT_Raster_Params renderer_;
  hb_font_t* font_;
  hb_font_t* big_font_;
  hb_buffer_t* buffer_;
  hb_unicode_funcs_t* unicode_funcs_;
  vector<hb_buffer_t*> segments_;
  vector<hb_script_t> scripts_;
};

Font::Font(const string& font_name, int font_size, FT_Library library)
    : library_(library) {
  LoadFace(font_name, font_size, library, &face_);
  LoadFace(font_name, 1.125*font_size, library, &big_face_);
  font_ = hb_ft_font_create(face_, nullptr);
  big_font_ = hb_ft_font_create(big_face_, nullptr);

  buffer_ = hb_buffer_create();
  unicode_funcs_ = hb_buffer_get_unicode_funcs(buffer_);
  ASSERT(unicode_funcs_ != nullptr,
         "Failed to get unicode_funcs for " << font_name);

  renderer_.target = nullptr;
  renderer_.flags = FT_RASTER_FLAG_DIRECT | FT_RASTER_FLAG_AA;
  renderer_.black_spans = nullptr;
  renderer_.bit_set = nullptr;
  renderer_.bit_test = nullptr;
}

Font::~Font() {
  hb_buffer_destroy(buffer_);
  for (auto& segment : segments_) {
    hb_buffer_destroy(segment);
  }
  hb_font_destroy(font_);
  hb_font_destroy(big_font_);
  FT_Done_Face(face_);
  FT_Done_Face(big_face_);
}

void Font::PrepareToRender(const string& text, Point* size, Point* baseline) {
  hb_buffer_clear_contents(buffer_);
  hb_buffer_set_direction(buffer_, HB_DIRECTION_LTR);
  hb_buffer_set_language(buffer_, hb_language_from_string("", 0));
  hb_buffer_add_utf8(buffer_, text.c_str(), text.size(), 0, text.size());
  SegmentBuffer(unicode_funcs_, buffer_, &segments_, &scripts_);

  SizerContext context;
  renderer_.user = &context;
  renderer_.gray_spans = Sizer;

  Point min_b(INT_MAX, INT_MAX);
  Point max_b(INT_MIN, INT_MIN);
  Point offset;

  for (int j = 0; j < segments_.size(); j++) {
    FT_Face face = (scripts_[j] == HB_SCRIPT_LATIN ? face_ : big_face_);
    hb_font_t* font = (scripts_[j] == HB_SCRIPT_LATIN ? font_ : big_font_);
    hb_buffer_set_script(segments_[j], scripts_[j]);
    hb_shape(font, segments_[j], nullptr, 0);

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info =
        hb_buffer_get_glyph_infos(segments_[j], &glyph_count);
    hb_glyph_position_t* glyph_pos =
        hb_buffer_get_glyph_positions(segments_[j], &glyph_count);

    for (int i = 0; i < glyph_count; i++) {
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
  }

  *size = max_b - min_b + Point(1, 1);
  baseline->x = -min_b.x;
  baseline->y = max_b.y;
}

void Font::Render(
    const Point& position, const Point& size, const Point& baseline,
    uint32_t color, SDL_Surface* surface, bool blend) {
  int x = position.x + baseline.x;
  int y = position.y + baseline.y;

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

  for (int j = 0; j < segments_.size(); j++) {
    FT_Face face = (scripts_[j] == HB_SCRIPT_LATIN ? face_ : big_face_);

    unsigned int glyph_count;
    hb_glyph_info_t* glyph_info =
        hb_buffer_get_glyph_infos(segments_[j], &glyph_count);
    hb_glyph_position_t* glyph_pos =
        hb_buffer_get_glyph_positions(segments_[j], &glyph_count);

    for (int i = 0; i < glyph_count; i++) {
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
  }

  SDL_UnlockSurface(surface);
}

}  // namespace font

namespace {
const static int kBitDepth = 32;
const static uint32_t kAMask = 0xff000000;
const static uint32_t kRMask = 0x00ff0000;
const static uint32_t kGMask = 0x0000ff00;
const static uint32_t kBMask = 0x000000ff;
}  // namespace

TextRenderer::TextRenderer(SDL_Renderer* renderer) : renderer_(renderer) {
  ASSERT(!FT_Init_FreeType(&library_), "Failed to initialize freetype!");
}

TextRenderer::~TextRenderer() {
  for (auto& pair : fonts_by_id_) {
    delete pair.second;
  }
  FT_Done_FreeType(library_);
}

Text* TextRenderer::DrawText(const string& font_name, int font_size,
                             const string& text) {
  Text* result = new Text;
  Font* font = LoadFont(font_name, font_size);
  font->PrepareToRender(text, &result->size, &result->baseline);

  // Create an appropriately-sized surface to render the text in.
  SDL_Surface* surface = SDL_CreateRGBSurface(
      0, result->size.x, result->size.y, kBitDepth,
      kAMask, kRMask, kGMask, kBMask);
  ASSERT(surface != nullptr, SDL_GetError());
  SDL_FillRect(surface, nullptr, 0x000000);

  font->Render(Point(0, 0), result->size, result->baseline,
               0xffffffff, surface);

  result->texture = SDL_CreateTextureFromSurface(renderer_, surface);
  ASSERT(result->texture != nullptr, SDL_GetError());
  SDL_FreeSurface(surface);
  return result;
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
