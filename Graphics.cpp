#include <string>

#include "constants.h"
#include "debug.h"
#include "util.h"
#include "Graphics.h"
#include "SDL_prims.h"

using std::string;
using std::vector;

namespace babel {

namespace {

static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kGridSize = 32;
static const int kTextSize = 0.6*kGridSize;

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

Graphics::Graphics(const Point& size) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);
  const Point dimensions(kGridSize*size);
  const Point grid(kGridSize, kGridSize);
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

void Graphics::Clear() {
  SDL_FillRect(buffer_->surface_, &buffer_->bounds_, 0x00000000);
}

void Graphics::Draw(const View& view) {
  // A collection of texts to draw. Layed out and drawn after all the tiles.
  vector<Point> positions;
  vector<string> texts;
  vector<SDL_Color> colors;
  // Fields needed to draw each cell.
  for (int x = 0; x < view.size; x++) {
    for (int y = 0; y < view.size; y++) {
      const TileView& tile = view.tiles[x][y];
      if (tile.graphic >= 0) {
        const Image* image =
            (tile.visible ? tileset_.get() : darkened_tileset_.get());
        image->Draw(Point(kGridSize*x, kGridSize*y), tile.graphic,
                    buffer_->bounds_, buffer_->surface_);
      }
    }
  }

  SDL_Color color;
  for (const SpriteView& sprite : view.sprites) {
    sprites_->Draw(kGridSize*sprite.square, sprite.graphic,
                   buffer_->bounds_, buffer_->surface_);
    if (!sprite.text.empty()) {
      ConvertColor(sprite.color, &color);
      positions.push_back(sprite.square);
      texts.push_back(sprite.text);
      colors.push_back(color);
    }
  }
  DrawTexts(positions, texts, colors);
  DrawLog(view.log);
  DrawStatus(view.status);
}

void Graphics::Flip() {
  SDL_Surface* surface = buffer_->surface_;
  SDL_UpdateTexture(texture_, nullptr, surface->pixels, surface->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

void Graphics::DrawTexts(const vector<Point>& positions,
                         const vector<string>& texts,
                         const vector<SDL_Color>& colors) {
  ASSERT(positions.size() == texts.size(), "Mismatched text size.");
  ASSERT(positions.size() == colors.size(), "Mismatched color size.");
  for (int i = 0; i < positions.size(); i++) {
    Direction dir = (positions[i].x <= kScreenRadius ?
                     Direction::LEFT : Direction::RIGHT);
    DrawText(positions[i].x, positions[i].y, dir, texts[i], colors[i]);
  }
}

void Graphics::DrawText(int x, int y, Direction dir,
                        const string& text, SDL_Color color) {
  const int margin = kGridSize/8;
  SDL_Rect rect{kGridSize*x, kGridSize*y + margin,
                kGridSize, kGridSize - 2*margin};
  text_renderer_->DrawTextBox(
      "default_font.ttf", kTextSize,
      text, rect, (Direction)dir, kBlack, color);
}

void Graphics::DrawLog(const vector<string>& log) {
  DrawDialogBox(log, true /* place_at_top */);
}

void Graphics::DrawStatus(const StatusView& status) {
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

}  // namespace babel
