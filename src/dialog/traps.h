#ifndef __BABEL_DIALOG_TRAP_H__
#define __BABEL_DIALOG_TRAP_H__

#include <vector>

#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Trap.h"

namespace babel {
namespace dialog {

class DialogGroupTrap : public engine::Trap {
 public:
  DialogGroupTrap(const std::vector<Point>& squares);

  void Trigger(engine::GameState* game_state,
               engine::EventHandler* handler) override;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_DIALOG_DIALOG_ACTION_H__
