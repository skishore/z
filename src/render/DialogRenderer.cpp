#include "render/DialogRenderer.h"

#include <algorithm>

#include "base/debug.h"
#include "base/point.h"

using std::find;
using std::max;
using std::pair;
using std::string;
using std::vector;

namespace babel {
namespace render {
namespace {

static const int kTextSize = 16;
static const int kCacheCapacity = 16;

}  // namespace

namespace dialog {

struct RenderParams {
  SDL_Renderer* renderer;
  DialogRenderer* text_renderer;
};

class Dialog {
 public:
  virtual ~Dialog() {
    for (Dialog* child : children_) {
      delete child;
    }
  }

  virtual void Draw(const SDL_Rect& rect, const RenderParams& params) const = 0;
  virtual int GetHeight() const = 0;

 protected:
  std::vector<Dialog*> children_;
  friend void AddChild(Dialog* parent, Dialog* child);
};

class ColumnDialog : public Dialog {
 public:
  int GetHeight() const override {
    int height = 0;
    for (Dialog* child : children_) {
      height = max(child->GetHeight(), height);
    }
    return height;
  }

  void Draw(const SDL_Rect& rect, const RenderParams& params) const override {
  }
};

class RowDialog : public Dialog {
 public:
  int GetHeight() const override {
    int height = 0;
    for (Dialog* child : children_) {
      height += child->GetHeight();
    }
    return height;
  }

  void Draw(const SDL_Rect& rect, const RenderParams& params) const override {
  }
};

class TextDialog : public Dialog {
 public:
  TextDialog(float size, const string& text, uint32_t color)
      : font_size_(size*kTextSize), text_(text), color_(color) {};

  int GetHeight() const override {
    return 3*font_size_/2;
  }

  void Draw(const SDL_Rect& rect, const RenderParams& params) const override {
  }

 private:
  const int font_size_;
  const string text_;
  const uint32_t color_;
};

void AddChild(Dialog* parent, Dialog* child) {
  parent->children_.push_back(child);
}

Dialog* MakeColumnDialog() {
  return new ColumnDialog;
}

Dialog* MakeRowDialog() {
  return new RowDialog;
}

Dialog* MakeTextDialog(int font_size, const string& text, uint32_t color) {
  return new TextDialog(font_size, text, color);
}

}  // namespace dialog

DialogRenderer::DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer)
    : bounds_(bounds), renderer_(renderer),
      text_renderer_(renderer), text_cache_(kCacheCapacity) {}

void DialogRenderer::Draw(const dialog::Dialog& dialog, bool place_at_top) {
}

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
