#include "render/DialogRenderer.h"

#include <algorithm>

#include "base/debug.h"
#include "base/point.h"

using std::find;
using std::pair;
using std::string;
using std::vector;

namespace babel {
namespace render {
namespace {

static const int kTextSize = 16;
static const int kCacheCapacity = 16;

}  // namespace

DialogRenderer::DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer)
    : bounds_(bounds), renderer_(renderer),
      text_renderer_(renderer), text_cache_(kCacheCapacity) {}

void DialogRenderer::DrawLines(const vector<string>& lines, bool place_at_top) {
  if (lines.empty()) {
    return;
  }
  const int border = 2;
  const int line_height = 3*kTextSize/2;
  const int margin = kTextSize/4;
  const Point padding(kTextSize, kTextSize/2);
  const int height = line_height*lines.size() + 2*border + 2*padding.y;

  SDL_Rect rect(bounds_);

  rect.x += margin;
  rect.y += (place_at_top ? margin : bounds_.h - height - margin);
  rect.h = height;
  rect.w -= 2*margin + 1;

  SDL_SetRenderDrawColor(renderer_, 0x00, 0x22, 0x66, 0xff);
  SDL_RenderFillRect(renderer_, &rect);
  SDL_SetRenderDrawColor(renderer_, 0xff, 0xff, 0xff, 0xff);
  for (int i = 0; i < border; i++) {
    SDL_RenderDrawRect(renderer_, &rect);
    rect.x += 1;
    rect.y += 1;
    rect.w -= 2;
    rect.h -= 2;
  }

  rect.x += padding.x;
  rect.y += padding.y + kTextSize;
  for (const string& line : lines) {
    Text* text = DrawText(kTextSize, line);
    const SDL_Rect dest{rect.x - text->baseline.x, rect.y - text->baseline.y,
                        text->size.x, text->size.y};
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(renderer_, text->texture, nullptr, &dest);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);

    rect.y += line_height;
  }
}

Text* DialogRenderer::DrawText(int font_size, const string& text) {
  pair<int,string> key{font_size, text};
  Text* result = text_cache_.Get(key);
  if (result == nullptr) {
    result = text_renderer_.DrawText("default_font.ttf", font_size, text);
    text_cache_.Set(key, result);
  }
  return result;
}

} // namespace render
} // namespace babel
