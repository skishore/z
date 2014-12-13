#ifndef TRANSLIT_HINDI_TRANSLITERATOR_H__
#define TRANSLIT_HINDI_TRANSLITERATOR_H__

#include <map>
#include <string>

#include "latin_to_hindi_map.h"

using std::map;
using std::string;

namespace translit {

class HindiTransliterator {
 public:
  HindiTransliterator();
  bool Process(const string& input, string* output);

 private:
  bool AdvanceState(char input, string* output);
  void PopState(string* output);

  string state_;
  const map<string, string>& map_ = kLatinToHindiMap;
};

}  // namespace translit

#endif  // TRANSLIT_HINDI_TRANSLITERATOR_H__
