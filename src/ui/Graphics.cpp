#include <algorithm>
#include <string>

#include "base/constants.h"
#include "base/debug.h"
#include "base/util.h"
#include "ui/Graphics.h"
#include "ui/SDL_prims.h"

using std::max;
using std::min;
using std::string;
using std::vector;

namespace babel {
namespace ui {

namespace {

static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kTextSize = 0.6*kGridSize;

// The number of squares around the edge that are NOT drawn.
static const int kPadding = 1;

inline void ConvertColor(const uint32_t color, SDL_Color* result) {
  result->r = (color >> 16) & 0xff;
  result->g = (color >> 8) & 0xff;
  result->b = color & 0xff;
}

}  // namespace

Graphics::DrawingSurface::DrawingSurface(const Point& size)
    : size_(size), bounds_{0, 0, size.x*kGridSize, size.y*kGridSize} {
  SDL_Surface* temp = SDL_CreateRGBSurface(
      0, bounds_.w, bounds_.h, kBitDepth, 0, 0, 0, 0);
  ASSERT(temp != nullptr, SDL_GetError());
  surface_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  ASSERT(surface_ != nullptr, SDL_GetError());
  SDL_FreeSurface(temp);
}

Graphics::DrawingSurface::~DrawingSurface() {
  SDL_FreeSurface(surface_);
}

Graphics::Graphics() {
  const int side = 2*(kScreenRadius - kPadding) + 1;
  const Point size(side, side);
  const Point dimensions(kGridSize*size);
  const Point grid(kGridSize, kGridSize);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);
  int status = SDL_CreateWindowAndRenderer(
      dimensions.x, dimensions.y, 0, &window_, &renderer_);
  ASSERT(status == 0, SDL_GetError());
  texture_ = SDL_CreateTexture(renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING,
                               dimensions.x, dimensions.y);
  ASSERT(texture_ != nullptr, SDL_GetError());

  buffer_.reset(new DrawingSurface(size));
  text_renderer_.reset(new TextRenderer(buffer_->bounds_, buffer_->surface_));

  tileset_.reset(new Image(grid, "tileset.bmp"));
  darkened_tileset_.reset(new Image(*tileset_, 0x88000000));
  sprites_.reset(new Image(grid, "sprites.bmp"));
}

Graphics::~Graphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void Graphics::Draw(const engine::View& view) {
  DrawInner(view, nullptr);
}

void Graphics::Draw(const engine::View& view, const Transform& transform) {
  DrawInner(view, &transform);
}

void Graphics::DrawInner(const engine::View& view, const Transform* transform) {
  Clear();

  Point camera_offset(kGridSize*kPadding, kGridSize*kPadding);
  if (transform != nullptr) {
    camera_offset += transform->camera_offset;
  }
  DrawTiles(view, camera_offset);

  // A collection of sprite text to draw. Layed out and drawn after the sprites.
  vector<Point> positions;
  vector<string> texts;
  vector<SDL_Color> colors;
  SDL_Color color;

  for (const auto& pair : view.sprites) {
    const engine::SpriteView& sprite = pair.second;
    Point sprite_offset;
    if (transform != nullptr) {
      if (transform->hidden_sprites.find(pair.first) !=
          transform->hidden_sprites.end()) {
        continue;
      }
      if (transform->sprite_offsets.find(pair.first) !=
          transform->sprite_offsets.end()) {
        sprite_offset = transform->sprite_offsets.at(pair.first);
      }
    }
    Point position = kGridSize*sprite.square + sprite_offset - camera_offset;
    DrawSprite(sprite, position);

    if (!sprite.text.empty()) {
      ConvertColor(sprite.color, &color);
      positions.push_back(position);
      texts.push_back(sprite.text);
      colors.push_back(color);
    }
  }

  if (transform != nullptr) {
    for (const auto& pair : transform->shaded_squares) {
      DrawShade(view, camera_offset, pair.first, pair.second);
    }
  }

  DrawTexts(positions, texts, colors);
  DrawLog(view.log);
  DrawStatus(view.status);

  Flip();
}

void Graphics::Clear() {
  SDL_FillRect(buffer_->surface_, &buffer_->bounds_, 0x00000000);
}

void Graphics::Flip() {
  SDL_Surface* surface = buffer_->surface_;
  SDL_UpdateTexture(texture_, nullptr, surface->pixels, surface->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

void Graphics::DrawTiles(const engine::View& view, const Point& offset) {
  for (int x = 0; x < view.size; x++) {
    for (int y = 0; y < view.size; y++) {
      const engine::TileView& tile = view.tiles[x][y];
      if (tile.graphic >= 0) {
        const Image* image =
            (tile.visible ? tileset_.get() : darkened_tileset_.get());
        const Point point = kGridSize*Point(x, y) - offset;
        image->Draw(point, tile.graphic, buffer_->bounds_, buffer_->surface_);
      }
    }
  }
}

void Graphics::DrawSprite(const engine::SpriteView& sprite,
                          const Point& position) {
  sprites_->Draw(position, sprite.graphic, buffer_->bounds_, buffer_->surface_);
}

void Graphics::DrawShade(
    const engine::View& view, const Point& offset,
    const Point& square, const Transform::Shade& shade) {
  SDL_Surface* surface = buffer_->surface_;
  const Point point = kGridSize*(square - view.offset) - offset;
  Uint32 color = shade.color;
  float alpha = shade.alpha;
  for (int j = max(point.x, 0);
       j < min(point.x + kGridSize, buffer_->bounds_.w); j++) {
    for (int k = max(point.y, 0);
         k < min(point.y + kGridSize, buffer_->bounds_.h); k++) {
      Uint32* pixel =
          (Uint32*)((char*)surface->pixels + surface->pitch*k + 4*j);
      Uint32 value = *pixel;
      #define R(color) ((color >> 16) & 0xff)
      #define G(color) ((color >> 8) & 0xff)
      #define B(color) (color & 0xff)
      *pixel = (((uint8_t)(alpha*R(color) + (1 - alpha)*R(value)) << 16) +
                ((uint8_t)(alpha*G(color) + (1 - alpha)*G(value)) << 8) +
                ((uint8_t)(alpha*B(color) + (1 - alpha)*B(value))));
      #undef R
      #undef G
      #undef B
    }
  }
}

void Graphics::DrawTexts(const vector<Point>& positions,
                         const vector<string>& texts,
                         const vector<SDL_Color>& colors) {
  ASSERT(positions.size() == texts.size(), "Mismatched text size.");
  ASSERT(positions.size() == colors.size(), "Mismatched color size.");
  for (int i = 0; i < positions.size(); i++) {
    Direction dir = (2*positions[i].x + kGridSize <= buffer_->bounds_.w ?
                     Direction::LEFT : Direction::RIGHT);
    DrawText(positions[i], dir, texts[i], colors[i]);
  }
}

void Graphics::DrawText(const Point& position, Direction dir,
                        const string& text, SDL_Color color) {
  const int margin = kGridSize/8;
  SDL_Rect rect{position.x, position.y + margin,
                kGridSize, kGridSize - 2*margin};
  text_renderer_->DrawTextBox(
      "default_font.ttf", kTextSize,
      text, rect, (Direction)dir, kBlack, color);
}

void Graphics::DrawLog(const vector<string>& log) {
  DrawDialogBox(log, true /* place_at_top */);
}

void Graphics::DrawStatus(const engine::StatusView& status) {
  vector<string> lines;
  lines.push_back(
      "Health: " + IntToString(status.cur_health) +
      "/" + IntToString(status.max_health));
  DrawDialogBox(lines, false /* place_at_top */);
}

void Graphics::DrawDialogBox(const vector<string>& lines, bool place_at_top) {
  if (lines.empty()) {
    return;
  }
  const int border = 2;
  const int font_size = 0.8*kTextSize;
  const int line_height = 3*font_size/2;
  const int margin = font_size/4;
  const Point padding(font_size, font_size/2);
  const int height = line_height*lines.size() + 2*border + 2*padding.y;

  SDL_Rect rect(buffer_->bounds_);

  rect.x += margin;
  rect.y += (place_at_top ? margin : buffer_->bounds_.h - height - margin);
  rect.h = height - 1;
  rect.w -= 2*margin+ 1;

  SDL_FillRect(buffer_->surface_, &rect, 0x00002266);
  for (int i = 0; i < border; i++) {
    SDL_DrawRect(buffer_->surface_, &rect, 0x00ffffff);
    rect.x += 1;
    rect.y += 1;
    rect.w -= 2;
    rect.h -= 2;
  }

  rect.x += padding.x;
  rect.y += padding.y;
  rect.h = line_height;
  for (const string& line : lines) {
    text_renderer_->DrawText("default_font.ttf", font_size, line, rect);
    rect.y += line_height;
  }
}

}  // namespace ui 
}  // namespace babel
