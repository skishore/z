#include <algorithm>
#include <memory>

#include "engine/Action.h"
#include "engine/Engine.h"
#include "engine/FieldOfVision.h"
#include "engine/Sprite.h"

using std::max;
using std::string;
using std::unique_ptr;

namespace babel {
namespace engine {

Engine::Engine() : game_state_("world.dat") {
  game_state_.log.AddLine(
      "Welcome to Babel! You are a neutral male human Padawan.");
  game_state_.log.Coalesce();
}

bool Engine::Update(Action* input, bool* used_input) {
  *used_input = false;
  bool changed = false;
  unique_ptr<Action> action;

  while (true) {
    if (!game_state_.player->IsAlive()) {
      break;
    }
    // Find the next sprite with enough energy to move.
    Sprite* sprite = game_state_.GetCurrentSprite();
    ASSERT(sprite != nullptr, "Current sprite is NULL!");
    if (!sprite->HasEnergyNeededToMove() && !sprite->GainEnergy()) {
      game_state_.AdvanceSprite();
      continue;
    }
    // Retrieve that sprite's next action.
    if (sprite->IsPlayer()) {
      if (input == nullptr || *used_input) {
        break;
      }
      action.reset(input);
      *used_input = true;
    } else {
      action.reset(sprite->GetAction(game_state_));
    }
    // Bind and execute the action and advance the sprite index.
    ActionResult result;
    while (action != nullptr) {
      action->Bind(sprite, &game_state_, nullptr);
      result = action->Execute();
      action.reset(result.alternate);
    }
    if (result.success || !sprite->IsPlayer()) {
      sprite->ConsumeEnergy();
      game_state_.AdvanceSprite();
      changed = true; 
    }
  }

  if (changed) {
    game_state_.log.Coalesce();
  }
  return changed;
}

}  // namespace engine
}  // namespace babel
