#include <vector>

#include "base/debug.h"
#include "interface/TransliterationGame.h"
#include "render/DialogRenderer.h"
#include "semantics/Devanagari.h"

using std::string;
using std::vector;

namespace babel {
namespace interface {

TransliterationGame::TransliterationGame() {
  const int num = (rand() % 4) + 3;
  for (int i = 0; i < num; i++) {
    const string hindi = semantics::Devanagari::GetRandomConjunct();
    segments_.push_back(hindi);
    answers_.push_back(semantics::Devanagari::HindiToEnglish(hindi));
    entries_.push_back("");
    guides_.push_back(false);
    length_ += hindi.size();
  }
  Advance();
}

DialogResult TransliterationGame::Consume(char ch) {
  DialogResult result;
  if (!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ch == ' ') ||
      index_ >= segments_.size()) {
    return result;
  }

  if (ch != answers_[index_][entries_[index_].size()]) {
    if (guides_[index_]) {
      return result;
    }
    entries_[index_].clear();
    guides_[index_] = true;
  } else {
    entries_[index_] += ch;
    Advance();
  }
  result.redraw = true;
  return result;
}

bool TransliterationGame::Active() const {
  return true;
}

void TransliterationGame::Draw(render::DialogRenderer* renderer) const {
  vector<string> lines{"To attack, you must transliterate: \u25AF"};
  renderer->DrawLines(lines, true /* place_at_top */);
}

void TransliterationGame::Advance() {
  while (index_ < segments_.size() &&
         entries_[index_].size() < answers_[index_].size()) {
    index_ += 1;
  }
}

}  // namespace interface
}  // namespace babel
