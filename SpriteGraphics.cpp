#include <string>

#include "constants.h"
#include "debug.h"
#include "SpriteGraphics.h"
#include "SDL_prims.h"

namespace skishore {

namespace {
static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kGridSize = 32;
static const int kTextSize = 0.5*kGridSize;
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

void SpriteGraphics::DrawTile(int x, int y, char tile) {
  const Image* image = tileset_.get();
  Point frame;
  if (tile == '@') {
    image->Draw(Point(kGridSize*x, kGridSize*y), frame,
                buffer_->bounds_, buffer_->surface_);
    image = player_.get();
    frame.x = 2;
  } else if (tile == 'X') {
    frame.x = 4;
  } else if (tile == '#') {
    frame.x = 5;
  } else if ('a' <= tile && tile <= 'z') {
    image->Draw(Point(kGridSize*x, kGridSize*y), frame,
                buffer_->bounds_, buffer_->surface_);
    image = enemy_.get();
    frame.x = 2;
  }

  image->Draw(Point(kGridSize*x, kGridSize*y), frame,
              buffer_->bounds_, buffer_->surface_);
}

void SpriteGraphics::DrawTileText(int x, int y, char tile) {
  if (tile == '@' || ('a' <= tile && tile <= 'z')) {
    int dir = (Direction)(abs(tile - 'a') % 3);
    if (dir == Direction::DOWN) {
      dir = Direction::LEFT;
    }
    SDL_Rect rect{kGridSize*x, kGridSize*y, kGridSize, kGridSize};
    text_renderer_->DrawTextBox(kTextSize, (Direction)dir, std::string({tile, tile, tile, tile, tile, tile, '\0'}), rect);
  }
}

void SpriteGraphics::Flip() {
  SDL_Surface* surface = buffer_->surface_;
  SDL_UpdateTexture(texture_, nullptr, surface->pixels, surface->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

}  // namespace skishore
