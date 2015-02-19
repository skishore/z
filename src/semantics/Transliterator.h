#ifndef __BABEL_SEMANTICS_TRANSLITERATOR_H__
#define __BABEL_SEMANTICS_TRANSLITERATOR_H__

#include <string>

namespace babel {
namespace semantics {

struct TransliterationResult {
  std::string error;
  std::string output;
};

class Transliterator {
 public:
  Transliterator(const std::string& input);

  const TransliterationResult& Run();

 protected:
  // This method should return true if the string is a valid character in the
  // input language of this transliterator. For example, for a Hindi-to-English
  // transliterator, it will be true if character is a valid Devanagari symbol.
  virtual bool IsValidCharacter(const std::string& character) = 0;

  // This method should return true if the string (which is a concatenation of
  // characters accepted by Accept) can be transliterated as a single unit.
  virtual bool IsValidState(const std::string& state) = 0;

  // This method should copy the current state into the transliteration output.
  virtual bool PopState() = 0;

 private:
  // Advances the end index until it contains a given character.
  bool GetNextCharacter();

  // Attempts to push the current character on to the state.
  bool AdvanceState();

  const std::string input_;
  std::string character_;
  std::string state_;
  TransliterationResult result_;

  // The start and end indices of the current character.
  int start_ = 0;
  int end_ = 0;
};

}  // namespace semantics
}  // namespace babel

#endif  // __BABEL_SEMANTICS_TRANSLITERATOR_H__
