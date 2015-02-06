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
  hindi_ = semantics::Devanagari::GetRandomConjunct();
  english_ = semantics::Devanagari::HindiToEnglish(hindi_);
}

DialogResult TransliterationGame::Consume(char ch) {
  DialogResult result;
  if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')) {
    input_ += ch;
    result.redraw = true;
  } else if (ch == '\b' && input_.size() > 0) {
    input_ = input_.substr(0, input_.size() - 1);
    result.redraw = true;
  }
  result.success = true;
  return result;
}

bool TransliterationGame::Active() const {
  return true;
}

void TransliterationGame::Draw(render::DialogRenderer* renderer) const {
  vector<string> lines{
      "To attack, you must transliterate " + hindi_ + ": " + input_ + "\u25AF"};
  renderer->DrawLines(lines, true /* place_at_top */);
}

}  // namespace interface
}  // namespace babel
