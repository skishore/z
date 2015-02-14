#include <stdlib.h>

#include "base/debug.h"
#include "semantics/devanagari.h"

using std::map;
using std::string;
using std::vector;

namespace babel {
namespace semantics {
namespace {

map<string,string> gHindiToEnglish;
map<string,string> gEnglishToHindi;

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

void InitializeHax() {
  if (gEnglishToHindi.size() > 0) {
    return;
  }
  ASSERT(Devanagari::alphabet.size() == kAlphabetZip.size(), "");
  for (int i = 0; i < Devanagari::alphabet.size(); i++) {
    gHindiToEnglish[Devanagari::alphabet[i]] = kAlphabetZip[i];
  }
}

template<typename T> vector<T> Concatenate(const vector<vector<T>>& lists) {
  vector<T> result;
  for (const vector<T>& list : lists) {
    for (const T& element : list) {
      result.push_back(element);
    }
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

}  // namespace

const vector<string> Devanagari::vowels{
  "अ",
  "आ",
  "इ",
  "ई",
  "उ",
  "ऊ",
  "ए",
  "ऐ",
  "ओ",
  "औ"
};

const vector<vector<string>> Devanagari::consonant_rows{
  {"क", "ख", "ग", "घ", "ङ"},
  {"च", "छ", "ज", "झ", "ञ"},
  {"ट", "ठ", "ड", "ढ", "ण"},
  {"त", "थ", "द", "ध", "न"},
  {"प", "फ", "ब", "भ", "म"},
  {"य", "र", "ल", "व"},
  {"श", "ष", "स", "ह"}
};

const vector<string> Devanagari::digits{
  "०",
  "१",
  "२",
  "३",
  "४",
  "५",
  "६",
  "७",
  "८",
  "९"
};

const map<string,string> Devanagari::vowel_to_sign{
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
  {"ॡ", "\u0963"},
  {"ँ", "ँ"},
  {"ं", "ं"},
  {"ः", "ः"}
};

const vector<string> Devanagari::consonants(Concatenate(consonant_rows));

const vector<string> Devanagari::alphabet(
    Concatenate(vector<vector<string>>{vowels, consonants}));

const vector<string> Devanagari::all(
    Concatenate(vector<vector<string>>{alphabet, digits}));

const map<string,string> Devanagari::sign_to_vowel(Invert(vowel_to_sign));

string Devanagari::GetRandomConjunct() {
  InitializeHax();
  const string& consonant = consonants[rand() % consonants.size()];
  const string& vowel = vowels[rand() % vowels.size()];
  const string& sign = vowel_to_sign.at(vowel);
  const string hi = consonant + sign;
  const string en = gHindiToEnglish.at(consonant) + gHindiToEnglish.at(vowel);
  gEnglishToHindi[en] = hi;
  gHindiToEnglish[hi] = en;
  return hi;
}

string Devanagari::EnglishToHindi(const string& english) {
  // TODO(skishore): Actually implement a transliterator here.
  return gEnglishToHindi.at(english);
}

string Devanagari::HindiToEnglish(const string& hindi) {
  // TODO(skishore): Actually implement a transliterator here.
  return gHindiToEnglish.at(hindi);
}

}  // namespace semantics
}  // namespace babel
