#include "semantics/Transliterator.h"

#include "base/util.h"

using std::string;

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

}  // namespace semantics
}  // namespace babel
