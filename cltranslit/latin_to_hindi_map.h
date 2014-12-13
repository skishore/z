#ifndef TRANSLIT_LATIN_TO_HINDI_MAP_H__
#define TRANSLIT_LATIN_TO_HINDI_MAP_H__

#include <map>
#include <string>

using std::map;
using std::string;

namespace translit {

const map<string, string> kLatinToHindiMap = {
  {"a", "अ"},
  {"aa", "आ"},
  {"A", "आ"},
  {"i", "इ"},
  {"ee", "ई"},
  {"ii", "ई"},
  {"I", "ई"},
  {"u", "उ"},
  {"oo", "ऊ"},
  {"U", "ऊ"},
  {"zr", "ऋ"},
  {"zl", "ऌ"},
  {"e", "ऍ"},
  {"E", "ए"},
  {"ai", "ऐ"},
  {"o", "ऑ"},
  {"O", "ओ"},
  {"au", "औ"},
  {"au", "औ"},
  {"zR", "ॠ"},
  {"zL", "ॡ"},
  {"AO", " ँ"},
  {"M", " ं"},
  {"H", " ः"},
  {"OM", "ॐ"}
};

}  // namespace translit

#endif  // TRANSLIT_LATIN_TO_HINDI_MAP_H__
