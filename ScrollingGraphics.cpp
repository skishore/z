#include "constants.h"
#include "debug.h"
#include "BattleData.h"
#include "ScrollingGraphics.h"

using std::string;

namespace skishore {

namespace {
static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kTextSize = 0.75*kGridSize;
}  // namespace

ScrollingGraphics::DrawingSurface::DrawingSurface(const Point& size)
    : size_(size), bounds_{0, 0, size.x*kGridSize, size.y*kGridSize} {
  SDL_Surface* temp = SDL_CreateRGBSurface(
      0, bounds_.w, bounds_.h, kBitDepth, 0, 0, 0, 0);
  ASSERT(temp != nullptr, SDL_GetError());
  surface_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  ASSERT(surface_ != nullptr, SDL_GetError());
  SDL_FreeSurface(temp);
}

ScrollingGraphics::DrawingSurface::~DrawingSurface() {
  SDL_FreeSurface(surface_);
}

ScrollingGraphics::ScrollingGraphics(const Point& size, const TileMap& map)
    : map_(map), cache_(kFormat) {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);
  const Point dimensions(kGridSize*size);

  int status = SDL_CreateWindowAndRenderer(
      dimensions.x, dimensions.y, 0, &window_, &renderer_);
  ASSERT(status == 0, SDL_GetError());
  USE_FOR_DEBUG(status);
  texture_ = SDL_CreateTexture(renderer_, kFormat, SDL_TEXTUREACCESS_STREAMING,
                               dimensions.x, dimensions.y);
  ASSERT(texture_ != nullptr, SDL_GetError());

  foreground_.reset(new DrawingSurface(size));
  background_.reset(new DrawingSurface(3*size));
  tileset_.reset(cache_.LoadImage(Point(kGridSize, kGridSize), "tileset.bmp"));

  text_renderer_.reset(new TextRenderer(
      foreground_->bounds_, foreground_->surface_));
  centered_ = SDL_Rect{dimensions.x/2, dimensions.y/2, 0, 0};
}

ScrollingGraphics::~ScrollingGraphics() {
  SDL_DestroyTexture(texture_);
  SDL_DestroyRenderer(renderer_);
  SDL_DestroyWindow(window_);
  SDL_Quit();
}

void ScrollingGraphics::DrawStatusMessage(const string& message) {
  text_renderer_->DrawText(kTextSize, Point(0, 0), message);
}

void ScrollingGraphics::CenterCamera(const Point& absolute_position) {
  const Point position = absolute_position - position_offset_;
  Point diff;

  if (position.x < centered_.x) {
    diff.x = position.x - centered_.x;
  } else if (position.x > centered_.x + centered_.w) {
    diff.x = position.x - (centered_.x + centered_.w);
  }
  if (position.y < centered_.y) {
    diff.y = position.y - centered_.y;
  } else if (position.y > centered_.y + centered_.h) {
    diff.y = position.y - (centered_.y + centered_.h);
  }

  if (diff.x != 0 || diff.y != 0) {
    camera_ += diff;
    position_offset_ += diff;
    const SDL_Rect& fg_bounds = foreground_->bounds_;
    const SDL_Rect& bg_bounds = background_->bounds_;
    if (camera_.x < 0 || camera_.x + fg_bounds.w > bg_bounds.w ||
        camera_.y < 0 || camera_.y + fg_bounds.h > bg_bounds.h) {
      Point offset_diff((camera_.x + (fg_bounds.w - bg_bounds.w)/2)/kGridSize,
                        (camera_.y + (fg_bounds.h - bg_bounds.h)/2)/kGridSize);
      background_offset_ += offset_diff;
      camera_ -= kGridSize*offset_diff;
      RedrawBackground();
    }
  }
}

void ScrollingGraphics::DrawSprite(const Sprite& sprite) {
  sprite.Draw(position_offset_, foreground_->bounds_, foreground_->surface_);
}

void ScrollingGraphics::DrawSpriteText(const Sprite& sprite) {
  if (sprite.battle_ != nullptr && !sprite.battle_->text.empty()) {
    Point position = sprite.GetDrawingPosition() - position_offset_;
    SDL_Rect rect {position.x, position.y, kGridSize, kGridSize};
    text_renderer_->DrawTextBox(
        kTextSize, sprite.battle_->dir, rect, sprite.battle_->text);
  }
}

void ScrollingGraphics::RedrawBackground() {
  Point frame;
  for (int x = 0; x < background_->size_.x; x++) {
    for (int y = 0; y < background_->size_.y; y++) {
      const Point square(x + background_offset_.x, y + background_offset_.y);
      frame.x = map_.GetMapTile(square);
      tileset_->Draw(Point(kGridSize*x, kGridSize*y), frame,
                     background_->bounds_, background_->surface_);
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

}  // namespace skishore
