#include <string>

#include "constants.h"
#include "debug.h"
#include "SpriteGraphics.h"
#include "SDL_prims.h"

using std::string;

namespace babel {

namespace {
static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kGridSize = 24;
static const int kTextSize = 0.8*kGridSize;
}  // namespace

SpriteGraphics::DrawingSurface::DrawingSurface(const Point& size)
    : size_(size), bounds_{0, 0, size.x*kGridSize, size.y*kGridSize} {
  SDL_Surface* temp = SDL_CreateRGBSurface(
      0, bounds_.w, bounds_.h, kBitDepth, 0, 0, 0, 0);
  ASSERT(temp != nullptr, SDL_GetError());
  surface_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  ASSERT(surface_ != nullptr, SDL_GetError());
  SDL_FreeSurface(temp);
}

SpriteGraphics::DrawingSurface::~DrawingSurface() {
  SDL_FreeSurface(surface_);
}

SpriteGraphics::SpriteGraphics(const Point& size) : cache_(kFormat) {
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
  tileset_.reset(cache_.LoadImage(Point(kGridSize, kGridSize), "tileset.bmp"));
  player_.reset(cache_.LoadImage(Point(kGridSize, kGridSize), "player.bmp"));
  enemy_.reset(cache_.LoadImage(Point(kGridSize, kGridSize), "troll.bmp"));
  text_renderer_.reset(new TextRenderer(buffer_->bounds_, buffer_->surface_));
}

SpriteGraphics::~SpriteGraphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void SpriteGraphics::Clear() {
  SDL_FillRect(buffer_->surface_, &buffer_->bounds_, 0x0);
}

void SpriteGraphics::DrawTile(int x, int y, char tile) {
  SDL_Color color{31, 31, 31};
  if (tile == '@') {
    color = SDL_Color{95, 255, 95};
  } else if ('a' <= tile && tile <= 'z') {
    color = SDL_Color{95, 95, 255};
    tile = 'X';
  } else if (tile == '.') {
    color = SDL_Color{95, 95, 95};
  } else if (tile == 'X') {
    color = SDL_Color{127, 127, 63};
    tile = '#';
  }
  SDL_Rect rect{kGridSize*x, kGridSize*y, kGridSize, kGridSize};
  text_renderer_->DrawText(
      "default_font.ttf", 0.9*kGridSize, std::string{tile}, rect, color);
}

void SpriteGraphics::DrawTileText(int x, int y, char tile) {
  if (tile == '@' || ('a' <= tile && tile <= 'z')) {
    int dir = (Direction)(abs(tile - 'a') % 2);
    if (dir == Direction::UP) {
      dir = Direction::LEFT;
    }
    SDL_Rect rect{kGridSize*x, kGridSize*y, kGridSize, kGridSize};
    const string text{tile, tile, tile, tile, tile, tile};
    text_renderer_->DrawTextBox(
        "Google Fonts/Noto_Sans/NotoSans-Regular.ttf", kTextSize,
        text, rect, (Direction)dir);
  }
}

void SpriteGraphics::Flip() {
  SDL_Surface* surface = buffer_->surface_;
  SDL_UpdateTexture(texture_, nullptr, surface->pixels, surface->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

}  // namespace babel
