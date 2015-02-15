#include "render/DialogRenderer.h"

#include <algorithm>
#include <memory>

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
  SDL_Rect rect;
  SDL_Renderer* renderer;
  DialogRenderer* text_renderer;
};

class Element {
 public:
  virtual ~Element() {
    for (Element* child : children_) {
      delete child;
    }
  }

  virtual int GetHeight() const = 0;

  virtual int GetWidth(const RenderParams& params) const {
    // The default implementation assumes a block-rendered element.
    return params.rect.w;
  }

  virtual void Draw(RenderParams params) const = 0;

 protected:
  std::vector<Element*> children_;
  friend void AddChild(Element* parent, Element* child);
};

class ColumnElement : public Element {
 public:
  int GetHeight() const override {
    int height = 0;
    for (Element* child : children_) {
      height += child->GetHeight();
    }
    return height;
  }

  void Draw(RenderParams params) const override {
    for (const Element* child : children_) {
      params.rect.h = child->GetHeight();
      child->Draw(params);
      params.rect.y += params.rect.h;
    }
  }
};

class RowElement : public Element {
 public:
  int GetHeight() const override {
    int height = 0;
    for (Element* child : children_) {
      height = max(child->GetHeight(), height);
    }
    return height;
  }

  void Draw(RenderParams params) const override {
    const int left = params.rect.x;
    const int width = params.rect.w;
    for (int i = 0; i < children_.size(); i++) {
      const Element* child = children_[i];
      params.rect.x = left + i*width/children_.size();
      params.rect.w = left + (i + 1)*width/children_.size() - params.rect.x;
      params.rect.h = child->GetHeight();
      child->Draw(params);
    }
  }
};

class SpanElement : public RowElement {
 public:
  SpanElement(bool centered) : centered_(centered) {}

  void Draw(RenderParams params) const override {
    vector<int> widths;
    int total_width = 0;
    for (Element* child : children_) {
      int width = child->GetWidth(params);
      widths.push_back(width);
      total_width += width;
    }
    if (centered_) {
      params.rect.x += (params.rect.w - total_width)/2;
      params.rect.w = total_width;
    }
    for (int i = 0; i < children_.size(); i++) {
      const Element* child = children_[i];
      params.rect.w = widths[i];
      params.rect.h = child->GetHeight();
      child->Draw(params);
      params.rect.x += params.rect.w;
    }
  }

 private:
  const bool centered_;
};

inline SDL_Color ConvertColor(uint32_t c) {
  return SDL_Color{uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c), 0x00};
}

class TextElement : public Element {
 public:
  TextElement(double size, const string& text, uint32_t fore, uint32_t back)
      : font_size_(size*kTextSize), text_(text),
        draw_fore_(fore != 0), draw_back_(back != 0),
        fore_(ConvertColor(fore)), back_(ConvertColor(back)) {}

  int GetHeight() const override {
    return 3*font_size_/2;
  }

  int GetWidth(const RenderParams& params) const {
    if (text_.empty()) {
      return 0;
    }
    Text* text = params.text_renderer->DrawText(font_size_, text_);
    return text->size.x;
  }

  void Draw(RenderParams params) const override {
    // NOTE: TextElements do NOT render any children that they may have.
    if (text_.empty()) {
      return;
    }
    Text* text = params.text_renderer->DrawText(font_size_, text_);
    const SDL_Rect dest{
        params.rect.x, params.rect.y - text->baseline.y + font_size_,
        text->size.x, text->size.y};
    if (draw_back_) {
      SDL_SetRenderDrawColor(params.renderer, back_.r, back_.g, back_.b, 0xff);
      SDL_RenderFillRect(params.renderer, &params.rect);
    }
    if (draw_fore_) {
      SDL_SetRenderDrawBlendMode(params.renderer, SDL_BLENDMODE_BLEND);
      SDL_SetTextureColorMod(text->texture, fore_.r, fore_.g, fore_.b);
      SDL_RenderCopy(params.renderer, text->texture, nullptr, &dest);
      SDL_SetTextureColorMod(text->texture, 0xff, 0xff, 0xff);
      SDL_SetRenderDrawBlendMode(params.renderer, SDL_BLENDMODE_NONE);
    }
  }

 private:
  const int font_size_;
  const string text_;
  const bool draw_fore_;
  const bool draw_back_;
  SDL_Color fore_;
  SDL_Color back_;
};

void AddChild(Element* parent, Element* child) {
  parent->children_.push_back(child);
}

Element* MakeColumnElement() {
  return new ColumnElement;
}

Element* MakeRowElement() {
  return new RowElement;
}

Element* MakeSpanElement(bool centered) {
  return new SpanElement(centered);
}

Element* MakeTextElement(double font_size, const string& text,
                         uint32_t fore, uint32_t back) {
  return new TextElement(font_size, text, fore, back);
}

}  // namespace dialog

DialogRenderer::DialogRenderer(const SDL_Rect& bounds, SDL_Renderer* renderer)
    : bounds_(bounds), renderer_(renderer),
      text_renderer_(renderer), text_cache_(kCacheCapacity) {}

void DialogRenderer::Draw(dialog::Element* element, bool place_at_top) {
  std::unique_ptr<dialog::Element> owner(element);
  const int border = 2;
  const int margin = kTextSize/4;
  const Point padding(kTextSize, kTextSize/2);
  const int height = element->GetHeight() + 2*border + 2*padding.y;

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
  rect.y += padding.y;
  rect.w -= 2*padding.x;
  rect.h -= 2*padding.y;
  element->Draw(dialog::RenderParams{rect, renderer_, this});
}

void DialogRenderer::DrawLines(const vector<string>& lines, bool place_at_top) {
  if (lines.empty()) {
    return;
  }
  dialog::Element* column = dialog::MakeColumnElement();
  for (const string& line : lines) {
    dialog::AddChild(column, dialog::MakeTextElement(1.0, line, 0x00ffffff));
  }
  Draw(column, place_at_top);
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
