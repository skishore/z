#include <vector>

#include "Sprite.h"

using std::string;
using std::vector;

namespace babel {

namespace {

static const int kFineness = 1 << 8;

bool AreAdjacent(const Sprite& sprite, const Sprite& other) {
  const Point diff = sprite.square - other.square;
  return (abs(diff.x) <= 1 && abs(diff.y) <= 1);
}

int ScoreMove(const Sprite& sprite, const GameState& game_state,
              const Point& move) {
  Point square = sprite.square + move;
  if ((move.x != 0 || move.y != 0) &&
      (game_state.map.IsSquareBlocked(square) ||
       game_state.IsSquareOccupied(square))) {
    return INT_MIN;
  }
  // Move toward the player if they are visible. Otherwise, move randomly.
  if (game_state.player_vision->IsSquareVisible(sprite.square)) {
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
  ASSERT(best_moves.size() > 0, "Could not find a move!");
  return best_moves[rand() % best_moves.size()];
}

}  // namespace

Sprite::Sprite(const Point& s, int type)
    : square(s), creature(kCreatures[type]) {
  if (type != kPlayerType) {
    text = string{(char)('A' + (rand() % 26))};
  }
}

void Sprite::Update(const GameState& game_state, SpriteAPI* api) {
  if (AreAdjacent(*this, *game_state.player)) {
    api->Attack(this, game_state.player);
  } else {
    api->Move(GetBestMove(*this, game_state), this);
  }
}

}  // namespace babel
