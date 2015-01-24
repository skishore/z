#ifndef __BABEL_SEMANTICS_DEVANAGARI_H__
#define __BABEL_SEMANTICS_DEVANAGARI_H__

#include <string>
#include <vector>

#include <base/util.h>

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

  static std::string GetRandomConjunct();
};

}  // namespace semantics
}  // namespace babel

#endif  // __BABEL_SEMANTICS_DEVANAGARI_H__
