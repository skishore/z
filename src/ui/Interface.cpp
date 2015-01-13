#include "base/debug.h"
#include "ui/Interface.h"

using std::string;
using std::vector;

namespace babel {
namespace ui {

void Interface::ClearLines() {
  has_lines_ = false;
}

bool Interface::Consume(char ch, engine::Action** action, bool* redraw) {
  // Temporarily disable the entire extended-command interface.
  return false;

  if (!speaking_) {
    if (ch == 's') {
      has_lines_ = true;
      speaking_ = true;
      speech_ = "";
      *redraw = true;
      return true;
    }
    return false;
  }

  string new_speech = speech_ + ch;
  if (engine::IsSpeechAllowed(new_speech)) {
    speech_ = new_speech;
    *redraw = true;
  } else if (ch == '\b') {
    if (!speech_.empty()) {
      speech_.erase(speech_.size() - 1);
      *redraw = true;
    }
  } else if (ch == '\n') {
    speaking_ = false;
    *action = new engine::SpeechAction(speech_);
    *redraw = true;
  }
  return true;
}

bool Interface::HasLines() const {
  return has_lines_;
}

vector<string> Interface::GetLines() const {
  ASSERT(HasLines(), "GetLines called when HasLines is false!");
  vector<string> result;
  if (speaking_) {
    result.push_back("What do you want to say? " + speech_);
  }
  return result;
}

}  // namespace ui
}  // namespace babel
