#include "interface/TransliterationMultipleChoiceGame.h"

#include <map>
#include <set>
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

const char kIndexToInput[] = {'a', 's', 'd', 'f'};

const std::map<char,int> kInputToIndex{
  {'a', 0},
  {'s', 1},
  {'d', 2},
  {'f', 3}
};

}  // namespace

TransliterationMultipleChoiceGame::TransliterationMultipleChoiceGame() {
  const int num = 4;
  correct_answer_ = rand() % num;
  std::set<string> transliterations;
  for (int i = 0; i < num; i++) {
    string hindi;
    string english;
    do {
      hindi = semantics::Devanagari::GetRandomConjunct();
      english = semantics::UnsafeHindiToEnglish(hindi);
    } while (transliterations.find(english) != transliterations.end());
    transliterations.insert(english);
    if (i == correct_answer_) {
      question_ = semantics::UnsafeHindiToEnglish(hindi);
    }
    answers_.push_back(hindi);
    attempted_.push_back(false);
  }
}

DialogResult TransliterationMultipleChoiceGame::Consume(char ch) {
  DialogResult result;
  if (!Active()) {
    pause_ -= 1;
    result.update = pause_ <= 0;
    return result;
  }

  if (kInputToIndex.find(ch) == kInputToIndex.end()) {
    return result;
  }
  const int index = kInputToIndex.at(ch);
  if (attempted_[index]) {
    return result;
  }

  attempted_[index] = true;
  if (index != correct_answer_) {
    errors_ += 1;
  }
  result.redraw_dialog = true;
  return result;
}

bool TransliterationMultipleChoiceGame::Active() const {
  return !attempted_[correct_answer_];
}

void TransliterationMultipleChoiceGame::Draw(
    render::DialogRenderer* renderer) const {
  Element* column = MakeColumnElement();
  AddChild(column, MakeTextElement(
      1.0, "To attack, you must transliterate \"" + Encode(question_) + "\":"));
  AddChild(column, MakeTextElement(0.2, ""));
  Element* row = MakeRowElement();
  for (int i = 0; i < answers_.size(); i++) {
    uint32_t color = 0xffffffff;
    if (attempted_[i]) {
      color = (i == correct_answer_ ? 0xff88ff44 : 0xffff8844);
    }
    Element* span = MakeSpanElement(true);
    const string text = "" + string(1, kIndexToInput[i]) + " - " + answers_[i];
    AddChild(span, MakeTextElement(1.4, text, color));
    AddChild(row, span);
  }
  AddChild(column, row);
  renderer->Draw(column, true /* place_at_top */);
}

GameResult TransliterationMultipleChoiceGame::GetResult() const {
  GameResult result;
  result.errors = errors_;
  return result;
}

}  // namespace interface
}  // namespace babel
