#include <algorithm>
#include <memory>

#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"

using std::max;
using std::string;
using std::vector;

namespace babel {
namespace engine {
namespace {

static const int kMaxSpeechSize = 16;
static const float kSpeechRadius = 3.2;

vector<Point> ComputeSquaresInRange(
    const GameState& game_state, const Sprite& sprite, int radius) {
  ASSERT(sprite.IsPlayer(), "ComputeSquaresInRange called for an NPC!");
  vector<Point> result;
  Point offset;
  for (offset.x = -kSpeechRadius; offset.x <= kSpeechRadius; offset.x++) {
    for (offset.y = -kSpeechRadius; offset.y <= kSpeechRadius; offset.y++) {
      const Point square = sprite.square + offset;
      if (game_state.player_vision->IsSquareVisible(square, kSpeechRadius)) {
        result.push_back(square);
      }
    }
  }
  return result;
}

bool IsSquareFree(const GameState& game_state, const Point& square) {
  return (!game_state.map.IsSquareBlocked(square) &&
          !game_state.IsSquareOccupied(square));
}

}  // namespace

bool IsSpeechAllowed(const string& text) {
  if (text.empty() || text.size() > kMaxSpeechSize) {
    return false;
  }
  for (int i = 0; i < text.size(); i++) {
    const char ch = text[i];
    if (ch == ' ') {
      if (i == 0 || text[i - 1] == ' ') {
        return false;
      }
    } else if (!(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'))) {
      return false;
    }
  }
  return true;
}

void Action::Bind(Sprite* sprite, GameState* game_state,
                  vector<EventHandler*>* handlers) {
  ASSERT(sprite_ == nullptr, "Bind was called twice!");
  ASSERT(sprite != nullptr, "sprite == nullptr");
  ASSERT(game_state != nullptr, "game_state == nullptr");
  ASSERT(handlers != nullptr, "handlers == nullptr");
  sprite_ = sprite;
  game_state_ = game_state;
  handlers_ = handlers;
}

AttackAction::AttackAction(Sprite* target) : target_(target) {}

ActionResult AttackAction::Execute() {
  ActionResult result;
  int damage = 0;
  for (int i = 0; i < sprite_->creature.attack.dice; i++) {
    damage += (rand() % sprite_->creature.attack.sides) + 1;
  }
  bool killed = damage >= target_->cur_health;

  if (sprite_->IsPlayer()) {
    const string verb = (killed ? "kill" : "hit");
    game_state_->log.AddLine(
        "You " + verb + " the " + target_->creature.appearance.name + ".");
  } else if (target_->IsPlayer()) {
    const string followup = (killed ? " You die..." : "");
    game_state_->log.AddLine(
        "The " + sprite_->creature.appearance.name + " hits!" + followup);
  }
  for (EventHandler* handler : *handlers_) {
    handler->BeforeAttack(*sprite_, *target_);
  }

  target_->cur_health = max(target_->cur_health - damage, 0);
  if (killed && !target_->IsPlayer()) {
    game_state_->RemoveNPC(target_);
  }
  result.success = true;
  return result;
}

MoveAction::MoveAction(const Point& move) : move_(move) {}

ActionResult MoveAction::Execute() {
  ActionResult result;
  Point square = sprite_->square + move_;
  if (square == sprite_->square) {
    result.success = true;
  } else if (IsSquareFree(*game_state_, square)) {
    game_state_->MoveSprite(move_, sprite_);
    result.success = true;
  } else if (game_state_->IsSquareOccupied(square)) {
    result.alternate = new AttackAction(game_state_->SpriteAt(square));
  }
  return result;
}

SpeechAction::SpeechAction(const string& text) : text_(text) {}

ActionResult SpeechAction::Execute() {
  ActionResult result;
  ASSERT(sprite_->IsPlayer(), "SpeechAction::Execute called for an NPC!");
  if (!IsSpeechAllowed(text_)) {
    return result;
  }

  game_state_->log.AddLine("You say: \"" + text_ + "\".");
  sprite_->text = text_;
  for (EventHandler* handler : *handlers_) {
    handler->BeforeSpeech(*sprite_);
  }
  result.success = true;
  return result;

  vector<Point> earshot = ComputeSquaresInRange(
      *game_state_, *sprite_, kSpeechRadius);
  for (const Point& square : earshot) {
    if (game_state_->IsSquareOccupied(square) && square != sprite_->square) {
      Sprite* sprite = game_state_->SpriteAt(square);
      if (rand() % 4 != 0) {
        const int damage = (rand() % 6) + 1;
        const bool killed = damage >= sprite->cur_health;
        const string verbs = (killed ? "kills" : "hurts");
        game_state_->log.AddLine("Your speech " + verbs + " the " +
                                 sprite->creature.appearance.name + "!");
        for (EventHandler* handler : *handlers_) {
          handler->BeforeAttack(*sprite_, *sprite);
        }
        sprite->cur_health = max(sprite->cur_health - damage, 0);
        if (killed) {
          game_state_->RemoveNPC(sprite);
        }
      }
    }
  }
  result.success = true;
  return result;
}

}  // namespace engine
}  // namespace babel
