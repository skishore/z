#include <algorithm>
#include <memory>

#include "Action.h"
#include "Engine.h"
#include "FieldOfVision.h"
#include "Sprite.h"

using std::max;
using std::string;
using std::unique_ptr;

namespace babel {

Engine::Engine() : game_state_("world.dat") {
  game_state_.log.AddLine(
      "Welcome to Babel! You are a neutral male human Padawan.");
  game_state_.log.Coalesce();
}

bool Engine::Update(bool has_input, char ch) {
  bool changed = false;
  unique_ptr<Action> action;
  while (true) {
    if (!game_state_.player->IsAlive()) {
      break;
    }
    Sprite* sprite = game_state_.GetCurrentSprite();
    ASSERT(sprite != nullptr, "Current sprite is NULL!");
    action.reset(sprite->GetAction(game_state_, ch, &has_input));
    if (action == nullptr) {
      // The sprite is the player and is waiting on input.
      break;
    }
    action->Bind(sprite);
    action->Execute(&game_state_);
    game_state_.AdvanceSprite();
    changed = true; 
  }
  if (changed) {
    game_state_.log.Coalesce();
  }
  return changed;
}

}  // namespace babel
