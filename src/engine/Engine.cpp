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
  handler_.handlers_.push_back(handler);
}

interface::Dialog* Engine::GetDialog() {
  return (interrupt_ == nullptr ? nullptr : interrupt_->dialog_.get());
}

bool Engine::Update(Action* input) {
  bool changed = false;
  unique_ptr<Action> action;

  game_state_.log.Open();

  while (true) {
    if (!game_state_.player->IsAlive() ||
        (interrupt_ != nullptr && interrupt_->dialog_->Active())) {
      break;
    }
    // Find the next sprite with enough energy to move.
    Sprite* sprite = game_state_.GetCurrentSprite();
    ASSERT(sprite != nullptr, "Current sprite is NULL!");
    if (!sprite->HasEnergyNeededToMove() && !sprite->GainEnergy()) {
      game_state_.AdvanceSprite();
      continue;
    }
    // Retrieve that sprite's next action. If we have a stored action that
    // triggered a UI-layer interrupt, use it; else, use the input action for
    // the player or an AI action for an NPC.
    if (interrupt_ != nullptr) {
      action.reset(interrupt_.release());
    } else if (sprite->IsPlayer()) {
      if (input == nullptr) {
        break;
      }
      action.reset(input);
      input = nullptr;
    } else {
      action.reset(sprite->GetAction(game_state_));
    }
    // Bind and execute the action and advance the sprite index.
    ActionResult result;
    while (action != nullptr) {
      action->Bind(sprite, &game_state_, &handler_);
      result = action->Execute();
      if (result.stalled) {
        ASSERT(result.alternate == nullptr, "Stalled Action has alternate!");
        ASSERT(action->dialog_  != nullptr, "Stalled Action has no dialog!");
        interrupt_.reset(action.release());
        changed = true;
        break;
      }
      action.reset(result.alternate);
    }
    // Using the action costs the sprite energy, unless it was a player action
    // that failed (such as a move into a blocked square).
    if (result.success || !sprite->IsPlayer()) {
      sprite->ConsumeEnergy();
      game_state_.AdvanceSprite();
      changed = true; 
    }
  }

  if (input != nullptr) {
    delete input;
  }
  game_state_.log.Flush(changed);
  return changed;
}

}  // namespace engine
}  // namespace babel
