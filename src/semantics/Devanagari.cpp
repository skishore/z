#include "semantics/Devanagari.h"

#include <stdlib.h>

#include "base/debug.h"
#include "base/util.h"

using std::map;
using std::string;
using std::vector;

namespace babel {
namespace semantics {

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

const vector<string> Devanagari::diacritics{
  "\u0901", // candrabindu
  "\u0902", // anusvara
  "\u0903"  // visarga
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
};

const vector<string> Devanagari::consonants(Concatenate(consonant_rows));

const vector<string> Devanagari::alphabet(
    Concatenate(vector<vector<string>>{vowels, consonants, diacritics}));

const vector<string> Devanagari::all(
    Concatenate(vector<vector<string>>{alphabet, digits}));

const map<string,string> Devanagari::sign_to_vowel(Invert(vowel_to_sign));

string Devanagari::GetRandomConjunct() {
  string consonant = consonants[rand() % consonants.size()];
  while (consonant == "ङ" || consonant == "ञ") {
    if (rand() % vowels.size() == 0) {
      return consonant;
    }
    consonant = consonants[rand() % consonants.size()];
  }
  const string& vowel = vowels[rand() % vowels.size()];
  const string& sign = vowel_to_sign.at(vowel);
  return consonant + sign;
}

}  // namespace semantics
}  // namespace babel
