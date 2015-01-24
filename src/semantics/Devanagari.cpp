#include <stdlib.h>

#include "base/util.h"
#include "semantics/devanagari.h"

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

const vector<string> Devanagari::consonants(Concatenate(consonant_rows));

const vector<string> Devanagari::alphabet(
    Concatenate(vector<vector<string>>{vowels, consonants}));

const vector<string> Devanagari::all(
    Concatenate(vector<vector<string>>{alphabet, digits}));

string Devanagari::GetRandomConjunct() {
  return (consonants[rand() % consonants.size()] +
          vowels[rand() % vowels.size()]);
}

}  // namespace semantics
}  // namespace babel
