#include <assert.h>

#include "ScrollingGraphics.h"
#include "Sprite.h"
#include "TileMap.h"

namespace skishore {

namespace {
static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
// The side length of each grid square, in pixels.
static const int kGridSize = 16;
}  // namespace

ScrollingGraphics::DrawingSurface::DrawingSurface(const Point& size)
    : size_(size), bounds_{0, 0, size.x*kGridSize, size.y*kGridSize} {
  SDL_Surface* temp = SDL_CreateRGBSurface(
      0, bounds_.w, bounds_.h, kBitDepth, 0, 0, 0, 0);
  assert(temp != nullptr);
  surface_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  assert(surface_ != nullptr);
  SDL_FreeSurface(temp);
}

ScrollingGraphics::DrawingSurface::~DrawingSurface() {
  SDL_FreeSurface(surface_);
}

ScrollingGraphics::ScrollingGraphics(const Point& size, const TileMap* map)
    : map_(map), cache_(kFormat) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);

  int status = SDL_CreateWindowAndRenderer(
      size.x*kGridSize, size.y*kGridSize, 0, &window_, &renderer_);
  assert(status == 0);
  texture_ = SDL_CreateTexture(renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING,
                               size.x*kGridSize, size.y*kGridSize);
  assert(texture_ != nullptr);

  foreground_.reset(new DrawingSurface(size));
  background_.reset(new DrawingSurface(Point(3*size.x, 3*size.y)));
  tileset_.reset(new Sprite(Point(kGridSize, kGridSize), &cache_));
  assert(tileset_->LoadImage("tileset.bmp"));
}

ScrollingGraphics::~ScrollingGraphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void ScrollingGraphics::CenterCamera(const Point& square) {
  RedrawBackground();
}

void ScrollingGraphics::EraseForeground() {
  const SDL_Rect& bounds = foreground_->bounds_;
  SDL_Rect source{camera_.x, camera_.y, bounds.w, bounds.h};
  SDL_Rect target(bounds);
  SDL_BlitSurface(background_->surface_, &source,
                  foreground_->surface_, &target);
}

void ScrollingGraphics::Flip() {
  SDL_Surface* surface = foreground_->surface_;
  SDL_UpdateTexture(texture_, nullptr, surface->pixels, surface->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

void ScrollingGraphics::RedrawBackground() {
  for (int x = 0; x < background_->size_.x; x++) {
    for (int y = 0; y < background_->size_.y; y++) {
      const Point square(x + background_offset_.x, y + background_offset_.y);
      tileset_->SetFrame(Point(map_->GetMapTile(square), 0));
      tileset_->SetPosition(Point(square.x*kGridSize, square.y*kGridSize));
      tileset_->Draw(background_->bounds_, background_->surface_);
    }
  }
}

}  // namespace skishore
