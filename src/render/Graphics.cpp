#include <algorithm>
#include <string>

#include "base/debug.h"
#include "base/util.h"
#include "render/Graphics.h"
#include "render/SDL_prims.h"

using std::map;
using std::max;
using std::min;
using std::string;
using std::vector;

namespace babel {
namespace render {
namespace {

static const Uint32 kFormat = SDL_PIXELFORMAT_ARGB8888;

// The number of squares around the edge that are NOT drawn.
static const int kPadding = 1;

}  // namespace

Graphics::DrawingSurface::DrawingSurface(
    const Point& size, SDL_Renderer* renderer)
    : size(size), bounds{0, 0, size.x*kGridSize, size.y*kGridSize} {
  texture = SDL_CreateTexture(renderer, kFormat, SDL_TEXTUREACCESS_STREAMING,
                              bounds.w, bounds.h);
  ASSERT(texture != nullptr, SDL_GetError());
}

Graphics::DrawingSurface::~DrawingSurface() {
  SDL_DestroyTexture(texture);
}

Graphics::Graphics(int radius, const InterfaceView& interface)
    : interface_(interface) {
  const int side = 2*(radius - kPadding) + 1;
  const Point size(side, side);
  const Point dimensions(kGridSize*size);
  const Point grid(kGridSize, kGridSize);

  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);
  int status = SDL_CreateWindowAndRenderer(
      dimensions.x, dimensions.y, 0, &window_, &renderer_);
  ASSERT(status == 0, SDL_GetError());

  SDL_RendererInfo info;
  ASSERT(SDL_GetRendererInfo(renderer_, &info) == 0, "Failed to get info!");
  DEBUG("Software: " << (info.flags & SDL_RENDERER_SOFTWARE));
  DEBUG("Accelerated: " << (info.flags & SDL_RENDERER_ACCELERATED));
  DEBUG("vsync: " << (info.flags & SDL_RENDERER_PRESENTVSYNC));
  DEBUG("Texture: " << (info.flags & SDL_RENDERER_TARGETTEXTURE));

  buffer_.reset(new DrawingSurface(size, renderer_));
  tileset_.reset(new Image(grid, "tileset.bmp", renderer_));
  darkened_tileset_.reset(new Image(*tileset_, 0x88000000, renderer_));
  sprites_.reset(new Image(grid, "sprites.bmp", renderer_));
}

Graphics::~Graphics() {
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
  SDL_SetRenderDrawColor(renderer_, 0x00, 0x00, 0x00, 0xff);
  SDL_RenderClear(renderer_);

  Point camera_offset(kGridSize*kPadding, kGridSize*kPadding);
  if (transform != nullptr) {
    camera_offset += transform->camera_offset;
  }
  DrawTiles(view, camera_offset);

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
    const Point position =
        kGridSize*sprite.square + sprite_offset - camera_offset;
    sprites_->Draw(position, sprite.graphic, buffer_->bounds, renderer_);
  }

  if (transform != nullptr) {
    for (const auto& pair : transform->shaded_squares) {
      DrawShade(view, camera_offset, pair.first, pair.second);
    }
  }

  if (interface_.HasLines()) {
    DEBUG("Not drawing interface lines.");
  }

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
        image->Draw(point, tile.graphic, buffer_->bounds, renderer_);
      }
    }
  }
}

void Graphics::DrawShade(
    const engine::View& view, const Point& offset,
    const Point& square, const Transform::Shade& shade) {
  const Point point = kGridSize*(square - view.offset) - offset;
  const SDL_Rect rect{
      max(point.x, 0), max(point.y, 0),
      max(min(kGridSize, buffer_->bounds.w - point.x), 0),
      max(min(kGridSize, buffer_->bounds.h - point.y), 0)};
  SDL_SetRenderDrawColor(
      renderer_, (shade.color >> 16) & 0xff, (shade.color >> 8) & 0xff,
      shade.color & 0xff, 0xff*shade.alpha);
  ASSERT(SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND) == 0,
         SDL_GetError());
  SDL_RenderFillRect(renderer_, &rect);
  ASSERT(SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE) == 0,
         SDL_GetError());
}

}  // namespace render
}  // namespace babel
