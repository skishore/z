#include "render/Image.h"

#include <algorithm>

using std::max;
using std::min;
using std::string;

namespace babel {
namespace render {

namespace {

static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;
static const int kBitDepth = 32;
static const int kActualSize = 16;
static const int kGridSize = 32;

SDL_Surface* CreateSurface(int w, int h) {
  SDL_Surface* temp = SDL_CreateRGBSurface(0, w, h, kBitDepth, 0, 0, 0, 0);
  ASSERT(temp != nullptr, "Failed to create temp: " << SDL_GetError());
  SDL_Surface* surface = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  SDL_FreeSurface(temp);
  ASSERT(surface != nullptr, "Failed to format surface: " << SDL_GetError());
  return surface;
}

void ApplyColorKey(uint32_t color, SDL_Surface* surface) {
  for (int x = 0; x < surface->w; x++) {
    for (int y = 0; y < surface->h; y++) {
      uint32_t* pixel =
          (uint32_t*)(((uint8_t*)surface->pixels) + y*surface->pitch + 4*x);
      if ((*pixel & 0x00ffffff) == color) {
        *pixel = *pixel & 0x00ffffff;
      }
    }
  }
  // Strangely, we still need to call SetColorKey.
  Uint32 color_key = SDL_MapRGB(surface->format, (color >> 16) & 0xff,
                                (color >> 8) & 0xff, color & 0xff);
  SDL_SetColorKey(surface, SDL_TRUE, color_key);
}

inline void Scale(int& value) {
  value = kActualSize*value/kGridSize;
}

inline void Scale(SDL_Rect& rect) {
  Scale(rect.x);
  Scale(rect.y);
  Scale(rect.w);
  Scale(rect.h);
}

}  // namespace

Image::Image(const Point& size, const string& filename, SDL_Renderer* renderer)
    : size_(size) {
  SDL_Surface* temp = SDL_LoadBMP(("images/" + filename).c_str());
  ASSERT(temp != nullptr,
         "Failed to load " << filename << ": " << SDL_GetError());
  surface_ = SDL_ConvertSurfaceFormat(temp, kFormat, 0);
  SDL_FreeSurface(temp);
  ASSERT(surface_ != nullptr,
         "Failed to format " << filename << ": " << SDL_GetError());

  // TODO(babel): At some point, we may want to apply the color key to
  // some images but not to others.
  ApplyColorKey(0x00ff00ff, surface_);
  texture_ = SDL_CreateTextureFromSurface(renderer, surface_);
  ASSERT(texture_ != nullptr, "Failed to load " << filename);
  DEBUG("Loaded " << filename);
}

Image::Image(const Image& image, Uint32 tint, SDL_Renderer* renderer)
    : size_(image.size_) {
  surface_ = CreateSurface(image.surface_->w, image.surface_->h);
  SDL_BlitSurface(image.surface_, nullptr, surface_, nullptr);
  SDL_Surface* tinter = CreateSurface(image.surface_->w, image.surface_->h);
  SDL_FillRect(tinter, nullptr, tint);
  ASSERT(!SDL_SetSurfaceBlendMode(tinter, SDL_BLENDMODE_BLEND), SDL_GetError());
  SDL_BlitSurface(tinter, nullptr, surface_, nullptr);
  SDL_FreeSurface(tinter);

  texture_ = SDL_CreateTextureFromSurface(renderer, surface_);
  ASSERT(texture_ != nullptr, "Failed to create tint texture.");
}

Image::~Image() {
  SDL_DestroyTexture(texture_);
  SDL_FreeSurface(surface_);
}

void Image::Draw(const Point& position, const Point& frame,
                 const SDL_Rect& bounds, SDL_Renderer* renderer) const {
  SDL_Rect source;
  SDL_Rect target;
  if (!PositionRects(position, bounds, &source, &target)) {
    return;
  }
  source.x += frame.x*size_.x;
  source.y += frame.y*size_.y;
  Scale(source);
  SDL_RenderCopy(renderer, texture_, &source, &target);
}

bool Image::PositionRects(const Point& position, const SDL_Rect& bounds,
                          SDL_Rect* source, SDL_Rect* target) const {
  const int& x = position.x;
  const int& y = position.y;

  // Don't draw the sprite if it is out of the target bounds.
  if ((x + size_.x < 0) or (x > bounds.w) or
      (y + size_.y < 0) or (y > bounds.h)) {
    return false;
  }

  // Place the target rect, taking care around the boundary.
  target->x = max(x, 0);
  target->y = max(y, 0);
  target->w = min(x + size_.x, bounds.w) - target->x;
  target->h = min(y + size_.y, bounds.h) - target->y;

  // Place the source rect based on the target's cutoffs.
  source->x = target->x - x;
  source->y = target->y - y;
  source->w = target->w;
  source->h = target->h;

  // Offset the target by the origin of the the bounding box.
  target->x += bounds.x;
  target->y += bounds.y;
  return true;
}

}  // namespace render
}  // namespace babel
