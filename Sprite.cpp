#include <vector>

#include "Sprite.h"

using std::string;
using std::vector;

namespace babel {

namespace {

int ScoreMove(const Sprite& sprite, const GameState& game_state,
              const Point& move) {
  Point square = sprite.square + move;
  if (game_state.map.IsSquareBlocked(square) ||
      game_state.IsSquareOccupied(square)) {
    return INT_MIN;
  }
  return 0;
  // Move toward the player if they are visible. Otherwise, move randomly.
  if (game_state.player_vision->IsSquareVisible(sprite.square)) {
    return -(game_state.player->square - square).length();
  }
  return 0;
}

}  // namespace

Sprite::Sprite(const Point& s, int type)
    : square(s), creature(kCreatures[type]) {
  if (type != kPlayerType) {
    text = string{(char)('A' + (rand() % 26))};
  }
}

Point Sprite::GetMove(const GameState& game_state) {
  vector<Point> best_moves;
  int best_score = INT_MIN;
  Point move;
  for (move.x = -1; move.x <= 1; move.x++) {
    for (move.y = -1; move.y <= 1; move.y++) {
      const int score = ScoreMove(*this, game_state, move);
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

}  // namespace babel
