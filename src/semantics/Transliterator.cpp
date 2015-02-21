#include "semantics/Transliterator.h"

#include "base/debug.h"
#include "base/util.h"
#include "semantics/Devanagari.h"

using std::map;
using std::string;
using std::vector;

namespace babel {
namespace semantics {

Transliterator::Transliterator(const std::string& input) : input_(input) {}

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

namespace {

const vector<string> kAlphabetZip{
  "a", "aa", "i", "ee", "u", "oo", "e", "ai", "o", "au",
  "k", "kh", "g", "gh", "NG",
  "ch", "Ch", "j", "jh", "NY",
  "T", "Th", "D", "Dh", "N",
  "t", "th", "d", "dh", "n",
  "p", "ph", "b", "bh", "m",
  "y", "r", "l", "v",
  "sh", "Sh", "s", "h"
};

const map<string,string> kVowelSigns{
  {"अ", ""},
  {"आ", "\u093E"},
  {"इ", "\u093F"},
  {"ई", "\u0940"},
  {"उ", "\u0941"},
  {"ऊ", "\u0942"},
  {"ऋ", "\u0943"},
  {"ऌ", "\u0962"},
  {"ऍ", "\u0946"},
  {"ऍ", "\u0946"},
  {"ए", "\u0947"},
  {"ऐ", "\u0948"},
  {"ऑ", "\u094A"},
  {"ओ", "\u094B"},
  {"औ", "\u094C"},
  {"ॠ", "\u0944"},
  {"ॡ", "\u0963"}
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

template<typename T> map<T,T> Invert(const map<T,T>& original) {
  map<T,T> result;
  for (const auto& pair : original) {
    ASSERT(result.find(pair.second) == result.end(),
           "Duplicate item found when computing inverted map: " << pair.second);
    result[pair.second] = pair.first;
  }
  return result;
}

const map<string,string> kEnglishToHindi =
    Zip(kAlphabetZip, Devanagari::alphabet);

}  // namespace

bool EnglishToHindiTransliterator::IsValidCharacter(
    const std::string& character) const {
  if (character.size() != 1) {
    return false;
  }
  char ch = character[0];
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}

bool EnglishToHindiTransliterator::IsValidState(
    const std::string& state) const {
  return state == "S" || kEnglishToHindi.find(state) != kEnglishToHindi.end();
}

void EnglishToHindiTransliterator::FinishWord() {
  last_was_consonant_ = false;
}

bool EnglishToHindiTransliterator::PopState() {
  string hindi = kEnglishToHindi.at(state_);
  const bool is_consonant = kVowelSigns.find(hindi) == kVowelSigns.end();
  if (last_was_consonant_) {
    if (is_consonant) {
      result_.output += kVirama;
    } else {
      hindi = kVowelSigns.at(hindi);
    }
  }
  result_.output += hindi;
  last_was_consonant_ = is_consonant;
  return true;
}

}  // namespace semantics
}  // namespace babel
