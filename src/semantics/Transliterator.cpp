#include "semantics/Transliterator.h"

#include "base/debug.h"
#include "base/util.h"
#include "semantics/Devanagari.h"

using std::map;
using std::string;
using std::vector;

namespace babel {
namespace semantics {
namespace {

// Non-injective mapping from Hindi characters to English letters.
// The reverse map must be computed by dictionary lookup, and there may
// still be ambiguities.
const vector<string> kAlphabetZip{
  "a", "A", "i", "I", "u", "U", "e", "ai", "o", "au",
  "k", "kh", "g", "gh", "ng",
  "ch", "chh", "j", "jh", "ny",
  "T", "Th", "D", "Dh", "n",
  "t", "th", "d", "dh", "n",
  "p", "ph", "b", "bh", "m",
  "y", "r", "l", "v",
  "sh", "sh", "s", "h",
  "ao", "n", "h"
};

const string kVirama = "\u094D";

template<typename T> map<T,T> Zip(
    const vector<T>& keys, const vector<T>& values) {
  map<T,T> result;
  ASSERT(keys.size() == values.size(), "Size mismatch!");
  for (int i = 0; i < keys.size(); i++) {
    ASSERT(result.find(keys[i]) == result.end(),
           "Duplicate item found when computing zip: " << keys[i]);
    result[keys[i]] = values[i];
  }
  return result;
}

const map<string,string> kHindiToEnglish =
    Zip(Devanagari::alphabet, kAlphabetZip);

inline bool IsDiacritic(const string& character) {
  return (character == "\u0901" || character == "\u0902" ||
          character == "\u0903");
}

inline bool Contains(const map<string,string> dict, const string& key) {
  return dict.find(key) != dict.end();
}

}  // namespace

Transliterator::Transliterator(const string& input) : input_(input) {}

const TransliterationResult& Transliterator::Run() {
  while (end_ < input_.size() && GetNextCharacter() && AdvanceState()) {
    start_ = end_;
  }
  if (!state_.empty() && result_.error.empty()) {
    PopState();
    state_.clear();
  }
  return result_;
}

bool Transliterator::GetNextCharacter() {
  while (end_ < input_.size()) {
    end_ += 1;
    character_ = input_.substr(start_, end_ - start_);
    if (character_ == " " || IsValidCharacter(character_)) {
      return true;
    }
  }
  result_.error = ("GetNextCharacter failed on input@" +
                   IntToString(start_) + ": " + input_.substr(start_));
  return false;
}

bool Transliterator::AdvanceState() {
  if (character_ == " ") {
    if (!state_.empty()) {
      PopState();
      state_.clear();
      FinishWord();
    }
    result_.output += " ";
    return true;
  }
  // This loop will only ever be traversed twice. The only way we can fail to
  // return is if we fall in the third case of the if statement, but since that
  // case pops and clears the state, we will never hit it twice in a row.
  while (true) {
    const string next_state = state_ + character_;
    if (IsValidState(next_state)) {
      state_ = next_state;
      return true;
    } else if (state_.empty()) {
      result_.error = ("Unexpected character at input@" +
                       IntToString(start_) + ": " + character_);
      return false;
    } else {
      PopState();
      state_.clear();
    }
  }
}

bool HindiToEnglishTransliterator::IsValidCharacter(
    const string& character) const {
  return (Contains(kHindiToEnglish, character) ||
          Contains(Devanagari::sign_to_vowel, character) ||
          character == kVirama);
}

bool HindiToEnglishTransliterator::IsValidState(const string& state) const {
  return IsValidCharacter(state);
}

void HindiToEnglishTransliterator::FinishWord() {
  last_was_consonant_ = false;
}

bool HindiToEnglishTransliterator::PopState() {
  const bool is_sign = Contains(Devanagari::sign_to_vowel, state_);
  if (is_sign || state_ == kVirama) {
    if (!last_was_consonant_) {
      result_.error = ("Unexpected conjunct at input@" +
                       IntToString(start_) + ": " + character_);
      return false;
    }
    if (is_sign) {
      result_.output += kHindiToEnglish.at(
          Devanagari::sign_to_vowel.at(state_));
    }
    last_was_consonant_ = false;
    return true;
  }
  if (last_was_consonant_) {
    // Add in the implicit schwa.
    result_.output += "a";
  }
  result_.output += kHindiToEnglish.at(state_);
  last_was_consonant_ =
      !(Contains(Devanagari::vowel_to_sign, state_) || IsDiacritic(state_));
  return true;
}

}  // namespace semantics
}  // namespace babel
