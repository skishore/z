#include "interface/Game.h"

#include <map>
#include <string>

using std::string;

namespace babel {
namespace interface {
namespace {

const std::map<char,string> kDiacritics{
  {'A', "ā"},
  {'I', "ī"},
  {'U', "ū"},
  {'D', "ḍ"},
  {'T', "ṭ"}
};

}  // namespace

string Game::Encode(const string& input) {
  string result;
  for (char ch : input) {
    if (kDiacritics.find(ch) != kDiacritics.end()) {
      result += kDiacritics.at(ch);
    } else {
      result += ch;
    }
  }
  return result;
};

}  // namespace interface
}  // namespace babel
