#ifndef __BABEL_INTERFACE_H__
#define __BABEL_INTERFACE_H__

#include <string>
#include <vector>

#include "engine/Action.h"

namespace babel {
namespace ui {

class Interface {
 public:
  void ClearLines();

  // Returns true if the input was consumed by the interface.
  // If the input is consumed, this method return an Action to take.
  bool Consume(char ch, engine::Action** action, bool* redraw);

  bool HasLines() const;
  std::vector<std::string> GetLines() const;

 private:
  bool has_lines_ = false;
  bool speaking_ = false;
  std::string speech_;
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_INTERFACE_H__
