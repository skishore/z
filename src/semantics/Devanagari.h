#ifndef __BABEL_SEMANTICS_DEVANAGARI_H__
#define __BABEL_SEMANTICS_DEVANAGARI_H__

#include <map>
#include <string>
#include <vector>

#include "base/util.h"

namespace babel {
namespace semantics {

class Devanagari {
 public:
  static const std::vector<std::string> vowels;
  static const std::vector<std::string> consonants;
  static const std::vector<std::string> digits;
  static const std::vector<std::string> alphabet;
  static const std::vector<std::string> all;

  static const std::vector<std::vector<std::string>> consonant_rows;
  static const std::map<std::string,std::string> vowel_to_sign;
  static const std::map<std::string,std::string> sign_to_vowel;

  static std::string GetRandomConjunct();
  static std::string EnglishToHindi(const std::string& english);
  static std::string HindiToEnglish(const std::string& hindi);
};

}  // namespace semantics
}  // namespace babel

#endif  // __BABEL_SEMANTICS_DEVANAGARI_H__
