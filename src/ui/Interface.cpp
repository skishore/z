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

  if (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z')) {
    speech_ += ch;
    *redraw = true;
  } else if (ch == '\b') {
    if (!speech_.empty()) {
      speech_.erase(speech_.size() - 1);
      *redraw = true;
    }
  } else if (ch == '\n') {
    // TODO(skishore): Set an Action here.
    speaking_ = false;
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
