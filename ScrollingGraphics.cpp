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

ScrollingGraphics::ScrollingGraphics(const Point& size, const TileMap* map)
    : size_(size), zone_size_(map->GetZoneSize()), map_(map),
      bounds_{0, 0, size_.x*kGridSize, size_.y*kGridSize},
      cache_(kFormat) {
  SDL_Init(SDL_INIT_VIDEO);
  int status = SDL_CreateWindowAndRenderer(
      bounds_.w, bounds_.h, 0, &window_, &renderer_);
  assert(status == 0);
  texture_ = SDL_CreateTexture(
      renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING, bounds_.w, bounds_.h);
  assert(texture_ != nullptr);

  SDL_Surface* temp = SDL_CreateRGBSurface(
      0, bounds_.w, bounds_.h, kBitDepth, 0, 0, 0, 0);
  assert(temp != nullptr);
  foreground_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  assert(foreground_ != nullptr);
  SDL_FreeSurface(temp);

  temp = SDL_CreateRGBSurface(
      0, zone_size_.x*kGridSize, zone_size_.y*kGridSize,
      kBitDepth, 0, 0, 0, 0);
  assert(temp != nullptr);
  background_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  assert(background_ != nullptr);
  SDL_FreeSurface(temp);

  SDL_ShowCursor(SDL_DISABLE);

  tileset_.reset(new Sprite(Point(kGridSize, kGridSize), &cache_));
  tileset_->LoadImage("tileset.bmp");
}

ScrollingGraphics::~ScrollingGraphics() {
  SDL_FreeSurface(background_);
  SDL_FreeSurface(foreground_);
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void ScrollingGraphics::RedrawBackground() {
  SDL_Rect bounds{0, 0, zone_size_.x*kGridSize, zone_size_.y*kGridSize};
  Point position;
  Point square;
  for (square.x = 0; square.x < zone_size_.x; square.x++) {
    for (square.y = 0; square.y < zone_size_.y; square.y++) {
      tileset_->SetFrame(Point(map_->GetZoneTile(square), 0));
      tileset_->SetPosition(position);
      tileset_->Draw(bounds, background_);
      position.y += kGridSize;
    }
    position.x += kGridSize;
    position.y = 0;
  }
}

void ScrollingGraphics::EraseForeground() {
  SDL_Rect source{camera_.x, camera_.y, bounds_.w, bounds_.h};
  SDL_Rect target(bounds_);
  SDL_BlitSurface(background_, &source, foreground_, &target);
}

void ScrollingGraphics::Flip() {
  SDL_UpdateTexture(texture_, nullptr, foreground_->pixels, foreground_->pitch);
  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}

}  // namespace skishore
