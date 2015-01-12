#include "base/debug.h"
#include "ui/Interface.h"

using std::string;
using std::vector;

namespace babel {
namespace ui {

bool Interface::Consume(char ch, engine::Action** action, bool* redraw) {
  if (!speaking_) {
    if (ch == 's') {
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
  return speaking_;
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
