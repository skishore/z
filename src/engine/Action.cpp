#include "engine/Action.h"

#include <algorithm>

#include "dialog/DialogAction.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using std::max;
using std::string;

namespace babel {
namespace engine {

void Action::Bind(Sprite* sprite, GameState* game_state,
                  EventHandler* handler) {
  ASSERT(sprite != nullptr);
  ASSERT(game_state != nullptr);
  ASSERT(handler != nullptr);
  // Check that Bind was not called twice for the same Action.
  ASSERT(sprite_ == nullptr);
  sprite_ = sprite;
  game_state_ = game_state;
  handler_ = handler;
}

AttackAction::AttackAction(Sprite* target) : target_(target) {}

ActionResult AttackAction::Execute() {
  ActionResult result;

  // Compute the attack base damage.
  int damage = 0;
  for (int i = 0; i < sprite_->creature->attack.dice; i++) {
    damage += (rand() % sprite_->creature->attack.sides) + 1;
  }

  // Exit early if the player is attacking an enemy with a combat dialog.
  if (sprite_->IsPlayer() &&
      dialog::DefendsWithDialog(*game_state_, *target_, damage)) {
    result.alternate = new dialog::LaunchDialogAction(target_);
    return result;
  }

  // Log and animate the attack.
  const bool killed = damage >= target_->cur_health;
  if (sprite_->IsPlayer()) {
    const string verb = (killed ? "kill" : "hit");
    game_state_->log.AddLine("You " + verb + " the " +
                             target_->creature->appearance.name + ".");
  } else {
    const string followup = (killed ? " You die..." : "");
    game_state_->log.AddLine(
        "The " + sprite_->creature->appearance.name + " hits!" + followup);
  }
  handler_->OnAttack(sprite_->Id(), target_->Id());

  // Execute the attack and maybe kill the sprite.
  target_->cur_health = max(target_->cur_health - damage, 0);
  if (!target_->IsPlayer() && killed) {
    game_state_->RemoveNPC(target_);
  }
  result.success = true;
  return result;
}

MoveAction::MoveAction(const Point& move) : move_(move) {}

ActionResult MoveAction::Execute() {
  ActionResult result;
  Point square = sprite_->square + move_;

  if (game_state_->map->IsSquareBlocked(square)) {
    if (sprite_->IsPlayer()) {
      result.alternate = new OpenDoorAction(square);
    }
    return result;
  }

  if (square == sprite_->square) {
    result.success = true;
  } else if (game_state_->IsSquareOccupied(square)) {
    result.alternate = new AttackAction(game_state_->SpriteAt(square));
  } else {
    game_state_->MoveSprite(move_, sprite_);
    if (sprite_->IsPlayer()) {
      game_state_->MaybeTriggerTrap(square, handler_);
    }
    result.success = true;
  }
  return result;
}

OpenDoorAction::OpenDoorAction(const Point& square) : square_(square) {}

ActionResult OpenDoorAction::Execute() {
  ActionResult result;
  const Tile tile = game_state_->map->GetTile(square_);

  if (tile == Tile::DOOR) {
    game_state_->map->SetTile(square_, Tile::FREE);
    game_state_->RecomputePlayerVision();
    // Check if we should log and animate the event.
    const int radius = game_state_->player->creature->stats.vision_radius;
    if (game_state_->player_vision->IsSquareVisible(square_, radius)) {
      const string verb_phrase =
          (sprite_->IsPlayer() ? "You open" :
           "The " + sprite_->creature->appearance.name + " opens");
      game_state_->log.AddLine(verb_phrase + " the door.");
      handler_->OnSnapshot();
    }
    result.success = true;
  }
  return result;
}

}  // namespace engine
}  // namespace babel
