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
  return false;
}

bool Interface::HasLines() const {
  return has_lines_;
}

vector<string> Interface::GetLines() const {
  ASSERT(HasLines(), "GetLines called when HasLines is false!");
  return vector<string>();
}

}  // namespace ui
}  // namespace babel
