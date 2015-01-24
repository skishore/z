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
      "Welcome to Babel! You are a neutral male human neophyte.");
  game_state_.log.Flush(true);
}

void Engine::AddEventHandler(EventHandler* handler) {
  ASSERT(handler != nullptr, "Added null EventHandler!");
  handlers_.push_back(handler);
}

bool Engine::Update(Action* input) {
  bool used_input = false;
  bool changed = false;
  unique_ptr<Action> action;

  game_state_.log.Open();

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
      if (input == nullptr || used_input) {
        break;
      }
      action.reset(input);
      used_input = true;
    } else {
      action.reset(sprite->GetAction(game_state_));
    }
    // Bind and execute the action and advance the sprite index.
    ActionResult result;
    while (action != nullptr) {
      action->Bind(sprite, &game_state_, &handlers_);
      result = action->Execute();
      action.reset(result.alternate);
    }
    if (result.success || !sprite->IsPlayer()) {
      sprite->ConsumeEnergy();
      game_state_.AdvanceSprite();
      changed = true; 
    }
  }

  if (input != nullptr && !used_input) {
    delete input;
  }
  game_state_.log.Flush(changed);
  return changed;
}

}  // namespace engine
}  // namespace babel
