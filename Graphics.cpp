#include <string>

#include "constants.h"
#include "debug.h"
#include "Graphics.h"
#include "SDL_prims.h"

using std::string;
using std::vector;

namespace babel {

namespace {

static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kGridSize = 24;
static const int kTextSize = 0.8*kGridSize;

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
  int status = SDL_CreateWindowAndRenderer(
      dimensions.x, dimensions.y, 0, &window_, &renderer_);
  ASSERT(status == 0, SDL_GetError());
  texture_ = SDL_CreateTexture(renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING,
                               dimensions.x, dimensions.y);
  ASSERT(texture_ != nullptr, SDL_GetError());

  buffer_.reset(new DrawingSurface(size));
  text_renderer_.reset(new TextRenderer(buffer_->bounds_, buffer_->surface_));
}

Graphics::~Graphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void Graphics::Clear() {
  SDL_FillRect(buffer_->surface_, &buffer_->bounds_, 0x000000);
}

void Graphics::DrawView(const View& view) {
  // A collection of texts to draw. Layed out and drawn after all the tiles.
  vector<Point> positions;
  vector<string> texts;
  vector<SDL_Color> colors;
  // Fields needed to draw each cell.
  SDL_Color color;
  char symbol;
  for (int x = 0; x < view.size; x++) {
    for (int y = 0; y < view.size; y++) {
      const TileView& tile = view.tiles[x][y];
      if (tile.sprite == nullptr) {
        symbol = tile.symbol;
        ConvertColor(tile.color, &color);
      } else {
        symbol = tile.sprite->symbol;
        ConvertColor(tile.sprite->color, &color);
        if (!tile.sprite->text.empty()) {
          positions.push_back(Point(x, y));
          texts.push_back(tile.sprite->text);
          colors.push_back(color);
        }
      }
      SDL_Rect rect{kGridSize*x, kGridSize*y, kGridSize, kGridSize};
      text_renderer_->DrawText("default_font.ttf", 0.9*kGridSize,
                               std::string{symbol}, rect, color);
    }
  }
  DrawTexts(positions, texts, colors);
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
  SDL_Rect rect{kGridSize*x, kGridSize*y, kGridSize, kGridSize};
  text_renderer_->DrawTextBox(
      "Google Fonts/Noto_Sans/NotoSans-Regular.ttf", kTextSize,
      text, rect, (Direction)dir, kBlack, kWhite);
}

}  // namespace babel
