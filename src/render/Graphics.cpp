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

// The number of squares around the edge that are NOT drawn.
static const int kPadding = 1;

}  // namespace

Graphics::Graphics(int radius, const interface::Dialog& dialog)
    : dialog_(dialog), bounds_{0, 0, 0, 0} {
  const int side = 2*(radius - kPadding) + 1;
  const Point size(side, side);
  const Point grid(kGridSize, kGridSize);
  bounds_.w = kGridSize*side;
  bounds_.h = kGridSize*side;

  SDL_Init(SDL_INIT_VIDEO);
  SDL_ShowCursor(SDL_DISABLE);
  int status = SDL_CreateWindowAndRenderer(
      bounds_.w, bounds_.h, 0, &window_, &renderer_);
  ASSERT(status == 0, SDL_GetError());

  SDL_RendererInfo info;
  ASSERT(SDL_GetRendererInfo(renderer_, &info) == 0, "Failed to get info!");
  DEBUG("Software: " << (info.flags & SDL_RENDERER_SOFTWARE));
  DEBUG("Accelerated: " << (info.flags & SDL_RENDERER_ACCELERATED));
  DEBUG("vsync: " << (info.flags & SDL_RENDERER_PRESENTVSYNC));
  DEBUG("Texture: " << (info.flags & SDL_RENDERER_TARGETTEXTURE));

  dialog_renderer_.reset(new DialogRenderer(bounds_, renderer_));
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
    sprites_->Draw(position, sprite.graphic, bounds_, renderer_);
  }

  if (transform != nullptr) {
    for (const auto& pair : transform->shaded_squares) {
      DrawShade(view, camera_offset, pair.first, pair.second);
    }
  }

  if (dialog_.Active()) {
    dialog_.Draw(dialog_renderer_.get());
  } else {
    dialog_renderer_->DrawLines(view.log, true /* place_at_top */);
  }
  vector<string> status;
  status.push_back(
      "Health: " + IntToString(view.status.cur_health) +
      "/" + IntToString(view.status.max_health));
  dialog_renderer_->DrawLines(status, false /* place_at_top */);

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
        image->Draw(point, tile.graphic, bounds_, renderer_);
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
      max(min(kGridSize, bounds_.w - point.x), 0),
      max(min(kGridSize, bounds_.h - point.y), 0)};
  SDL_SetRenderDrawColor(
      renderer_, (shade.color >> 16) & 0xff, (shade.color >> 8) & 0xff,
      shade.color & 0xff, 0xff*shade.alpha);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
  SDL_RenderFillRect(renderer_, &rect);
  SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
}

}  // namespace render
}  // namespace babel
