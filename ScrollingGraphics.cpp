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
// The fraction of the view that is contained within the centered box.
static const double kBoxFraction = 0.125;
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
  const Point dimensions(kGridSize*size);

  int status = SDL_CreateWindowAndRenderer(
      dimensions.x, dimensions.y, 0, &window_, &renderer_);
  assert(status == 0);
  texture_ = SDL_CreateTexture(renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING,
                               dimensions.x, dimensions.y);
  assert(texture_ != nullptr);

  foreground_.reset(new DrawingSurface(size));
  background_.reset(new DrawingSurface(3*size));
  tileset_.reset(new Sprite(Point(kGridSize, kGridSize), &cache_));
  assert(tileset_->LoadImage("tileset.bmp"));

  int box_w = kBoxFraction*dimensions.x;
  int box_h = kBoxFraction*dimensions.y;
  centered_ = SDL_Rect{
      (dimensions.x - box_w)/2, (dimensions.y - box_h)/2, box_w, box_h};
}

ScrollingGraphics::~ScrollingGraphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void ScrollingGraphics::CenterCamera(const Point& square) {
  // The square is given in map coordinates. Convert it to screen coordinates.
  const Point position = kGridSize*(square - background_offset_) - camera_;
  const Point dimensions(kGridSize, kGridSize);
  Point diff;

  if (position.x < centered_.x) {
    diff.x = position.x - centered_.x;
  } else if (position.x + dimensions.x > centered_.x + centered_.w) {
    diff.x = position.x + dimensions.x > centered_.x + centered_.w;
  }
  if (position.y < centered_.y) {
    diff.x = position.y - centered_.y;
  } else if (position.y + dimensions.y > centered_.y + centered_.h) {
    diff.x = position.y + dimensions.y > centered_.y + centered_.h;
  }

  if (diff.x != 0 || diff.y != 0) {
    camera_ += diff;
    const SDL_Rect& fg_bounds = foreground_->bounds_;
    if (camera_.x < 0 || camera_.x + fg_bounds.w > background_->bounds_.w ||
        camera_.y < 0 || camera_.y + fg_bounds.h > background_->bounds_.h) {
    }
  }
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
      tileset_->SetPosition(kGridSize*square);
      tileset_->Draw(background_->bounds_, background_->surface_);
    }
  }
}

}  // namespace skishore
