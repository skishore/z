#include "engine/Engine.h"

#include <algorithm>
#include <memory>

#include "base/debug.h"
#include "engine/Action.h"
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

Engine::~Engine() {
  for (auto* input : inputs_) {
    delete input;
  }
}

void Engine::AddEventHandler(EventHandler* handler) {
  ASSERT(handler != nullptr);
  handler_.handlers_.push_back(handler);
}

void Engine::AddInput(Action* input) {
  ASSERT(input != nullptr);
  if (inputs_.empty() || input->Queueable()) {
    inputs_.push_back(input);
  }
}

bool Engine::Update() {
  bool changed = false;
  unique_ptr<Action> action;

  game_state_.log.Open();

  while (true) {
    if (!game_state_.player->IsAlive()) {
      break;
    }
    // Find the next sprite with enough energy to move.
    Sprite* sprite = game_state_.GetCurrentSprite();
    ASSERT(sprite != nullptr);
    if (!sprite->HasEnergyNeededToMove() && !sprite->GainEnergy()) {
      game_state_.AdvanceSprite();
      continue;
    }
    // Retrieve that sprite's next action, pulling from the input actions for
    // the player or getting an AI action for an NPC.
    if (sprite->IsPlayer()) {
      if (inputs_.size() == 0) {
        break;
      }
      action.reset(inputs_.back());
      inputs_.pop_back();
    } else {
      action.reset(sprite->GetAction(game_state_));
    }
    // Bind and execute the action and advance the sprite index.
    ActionResult result;
    while (action != nullptr) {
      action->Bind(sprite, &game_state_, &handler_);
      result = action->Execute();
      if (result.stalled) {
        // Stalled actions should not return alternates. They will not be executed.
        ASSERT(result.alternate == nullptr);
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

  for (auto* input : inputs_) {
    delete input;
  }
  inputs_.clear();
  game_state_.log.Flush(changed);
  return changed;
}

View* Engine::GetView(const Point& size) const {
  return new View(size, game_state_);
}

}  // namespace engine
}  // namespace babel
