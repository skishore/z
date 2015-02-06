#include <map>

#include "base/debug.h"
#include "base/point.h"
#include "engine/Action.h"
#include "ui/Interface.h"

using std::map;

namespace babel {
namespace ui {
namespace {

static const std::map<char,Point> kShift = {
  {'h', Point(-1, 0)},
  {'j', Point(0, 1)},
  {'k', Point(0, -1)},
  {'l', Point(1, 0)},
  {'y', Point(-1, -1)},
  {'u', Point(1, -1)},
  {'b', Point(-1, 1)},
  {'n', Point(1, 1)},
  {'.', Point(0, 0)}
};

}  // namespace

void Interface::Register(engine::Engine* engine) {
  ASSERT(engine != nullptr, "engine_ == nullptr!");
  engine_ = engine;
  Clear();
}

void Interface::Clear() {
  active_ = false;
}

interface::DialogResult Interface::Consume(char ch) {
  interface::Dialog* dialog = engine_->GetDialog();
  if (dialog != nullptr) {
    return dialog->Consume(ch);
  }

  interface::DialogResult result;
  if (ch == 'r') {
    result.reset = true;
  } else if (kShift.find(ch) != kShift.end()) {
    result.action = new engine::MoveAction(kShift.at(ch));
  }
  return result;
}

bool Interface::Active() const {
  return (active_ || engine_->GetDialog() != nullptr);
}

void Interface::Draw(render::DialogRenderer* renderer) const {
  ASSERT(Active(), "Draw called when the dialog was inactive!");
  interface::Dialog* dialog = engine_->GetDialog();
  if (dialog != nullptr) {
    dialog->Draw(renderer);
  }
}

}  // namespace ui
}  // namespace babel
