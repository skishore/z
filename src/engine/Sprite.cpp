#include "engine/Sprite.h"

#include <vector>

#include "engine/Action.h"
#include "engine/GameState.h"

using std::string;
using std::vector;

namespace babel {
namespace engine {

namespace {

static const int kFineness = 1 << 8;
static const int kEnergyNeededToMove = 240;
// Global counter used to assign sprites unique ids.
static sid gIdCounter;

bool AreAdjacent(const Sprite& sprite, const Sprite& other) {
  const Point diff = sprite.square - other.square;
  return (abs(diff.x) <= 1 && abs(diff.y) <= 1);
}

int ScoreMove(const Sprite& sprite, const GameState& game_state,
              const Point& move) {
  Point square = sprite.square + move;
  if ((move.x != 0 || move.y != 0) &&
      (game_state.map->IsSquareBlocked(square) ||
       game_state.IsSquareOccupied(square))) {
    return INT_MIN;
  }
  // Move toward the player if they are visible. Otherwise, move randomly.
  const int radius = sprite.creature->stats.vision_radius;
  if (game_state.player_vision->IsSquareVisible(sprite.square, radius)) {
    return -kFineness*(game_state.player->square - square).length();
  }
  return 0;
}

const Point GetBestMove(const Sprite& sprite, const GameState& game_state) {
  vector<Point> best_moves;
  int best_score = INT_MIN;
  Point move;
  for (move.x = -1; move.x <= 1; move.x++) {
    for (move.y = -1; move.y <= 1; move.y++) {
      const int score = ScoreMove(sprite, game_state, move);
      if (score > best_score) {
        best_score = score;
        best_moves.clear();
        best_moves.push_back(move);
      } else if (score == best_score) {
        best_moves.push_back(move);
      }
    }
  }
  ASSERT(best_moves.size() > 0);
  return best_moves[rand() % best_moves.size()];
}

}  // namespace

Sprite::Sprite(const Point& s, int t) : square(s),  id(gIdCounter) {
  gIdCounter += 1;
  Polymorph(t);
  if (IsPlayer()) {
    energy = kEnergyNeededToMove;
  } else {
    energy = rand() % kEnergyNeededToMove;
  }
}

bool Sprite::HasEnergyNeededToMove() const {
  return energy >= kEnergyNeededToMove;
}

bool Sprite::GainEnergy() {
  energy += creature->stats.speed;
  return energy >= kEnergyNeededToMove;
}

void Sprite::ConsumeEnergy() {
  energy -= kEnergyNeededToMove;
}

Action* Sprite::GetAction(const GameState& game_state) const {
  ASSERT(!IsPlayer());
  if (AreAdjacent(*this, *game_state.player)) {
    return new AttackAction(game_state.player);
  } else {
    return new MoveAction(GetBestMove(*this, game_state));
  }
}

void Sprite::Polymorph(int t) {
  type = t;
  creature = &kCreatures[t];
  max_health = creature->stats.max_health;
  cur_health = max_health;
}

bool Sprite::IsAlive() const {
  return cur_health > 0;
}

bool Sprite::IsPlayer() const {
  return type == mPlayer;
}

}  // namespace engine
}  // namespace babel
