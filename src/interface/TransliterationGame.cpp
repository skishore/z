#include "interface/TransliterationGame.h"

#include <map>
#include <vector>

#include "base/debug.h"
#include "render/DialogRenderer.h"
#include "semantics/Devanagari.h"
#include "semantics/Transliterator.h"

using std::string;
using std::vector;
using babel::render::dialog::AddChild;
using babel::render::dialog::Element;
using babel::render::dialog::MakeColumnElement;
using babel::render::dialog::MakeRowElement;
using babel::render::dialog::MakeSpanElement;
using babel::render::dialog::MakeTextElement;

namespace babel {
namespace interface {
namespace {

const std::map<char,string> kDiacritics{
  {'A', "ā"},
  {'I', "ī"},
  {'U', "ū"},
  {'D', "ḍ"},
  {'T', "ṭ"}
};

// Encodes a Latin string using the diacritics map above
string Encode(const string& input) {
  string result;
  for (char ch : input) {
    if (kDiacritics.find(ch) != kDiacritics.end()) {
      result += kDiacritics.at(ch);
    } else {
      result += ch;
    }
  }
  return result;
};

}  // namespace

TransliterationGame::TransliterationGame() {
  const int num = (rand() % 2) + 2;
  for (int i = 0; i < num; i++) {
    const string hindi = semantics::Devanagari::GetRandomConjunct();
    segments_.push_back(hindi);
    answers_.push_back(semantics::UnsafeHindiToEnglish(hindi));
    entries_.push_back("");
    guides_.push_back(false);
    length_ += hindi.size();
  }
  Advance();
}

DialogResult TransliterationGame::Consume(char ch) {
  DialogResult result;
  if (index_ >= segments_.size()) {
    pause_ -= 1;
    result.update = pause_ <= 0;
    return result;
  }

  if (!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == ' ')) {
    return result;
  }

  if (ch != answers_[index_][entries_[index_].size()]) {
    if (guides_[index_]) {
      return result;
    }
    entries_[index_].clear();
    guides_[index_] = true;
    errors_ += 1;
  } else {
    entries_[index_] += ch;
    Advance();
  }
  result.redraw_dialog = true;
  return result;
}

bool TransliterationGame::Active() const {
  return index_ < segments_.size();
}

void TransliterationGame::Draw(render::DialogRenderer* renderer) const {
  Element* column = MakeColumnElement();
  AddChild(column, MakeTextElement(1.0, "To attack, you must transliterate:"));
  AddChild(column, MakeTextElement(0.4, ""));
  Element* top = MakeRowElement();
  Element* bottom = MakeRowElement();
  for (int i = 0; i < segments_.size(); i++) {
    const uint32_t color = (i < index_ ? 0xff88ff44 : 0xffffffff);
    const uint32_t guide = 0xffcccccc;
    const uint32_t cursor = 0xff666666;
    Element* top_span = MakeSpanElement(true);
    AddChild(top_span, MakeTextElement(1.8, segments_[i], color));
    Element* bottom_span = MakeSpanElement(true);
    AddChild(bottom_span, MakeTextElement(1.0, Encode(entries_[i]), color));
    if (guides_[i]) {
      string leftover = answers_[i].substr(entries_[i].size());
      if (i == index_ && leftover.size() > 0) {
        const string ch(1, leftover[0]);
        leftover = leftover.substr(1);
        AddChild(bottom_span, MakeTextElement(1.0, Encode(ch), guide, cursor));
      }
      if (leftover.size() > 0) {
        AddChild(bottom_span, MakeTextElement(1.0, Encode(leftover), guide));
      }
    } else if (i == index_) {
      AddChild(bottom_span, MakeTextElement(1.0, "A", 0x0, cursor));
    }
    AddChild(top, top_span);
    AddChild(bottom, bottom_span);
  }
  AddChild(column, top);
  AddChild(column, bottom);
  renderer->Draw(column, true /* place_at_top */);
}

void TransliterationGame::Advance() {
  while (index_ < segments_.size() &&
         entries_[index_].size() >= answers_[index_].size()) {
    index_ += 1;
  }
}

}  // namespace interface
}  // namespace babel
