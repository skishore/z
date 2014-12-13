#include "hindi_transliterator.h"

#include <assert.h>
#include <string>

#include "latin_to_hindi_map.h"

using std::string;

namespace translit {

const 

HindiTransliterator::HindiTransliterator() {}

bool HindiTransliterator::Process(const string& input, string* output) {
  for (int i = 0; i < input.size(); i++) {
    if (!AdvanceState(input[i], output)) {
      return false;
    }
  }
  PopState(output);
  return true;
}

bool HindiTransliterator::AdvanceState(char input, string* output) {
  if (('a' <= input && input <= 'z') || ('A' <= input && input <= 'Z')) {
    const string next_state = state_ + input;
    if (map_.find(next_state) != map_.end()) {
      state_ = next_state;
    } else if (state_.empty()) {
      return false;
    } else {
      PopState(output);
      return AdvanceState(input, output);
    }
  } else {
    PopState(output);
    *output += input;
  }
  return true;
}

void HindiTransliterator::PopState(string* output) {
  if (!state_.empty()) {
    auto iterator = map_.find(state_);
    assert(iterator != map_.end());
    *output += iterator->second;
    state_.clear();
  }
}

}  // namespace translit
