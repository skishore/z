#ifndef BABEL_ENGINE_H__
#define BABEL_ENGINE_H__

#include "GameState.h"
#include "Point.h"
#include "Sprite.h"
#include "View.h"

namespace babel {

class Engine : public SpriteAPI {
 public:
  Engine();

  const GameState& GetGameState() const { return game_state_; }

  // Returns true if the command was valid.
  // If this method returns false, the view does not need to be redrawn.
  bool HandleCommand(char command);

  // SpriteAPI interface methods used to update sprites.
  void Attack(Sprite* sprite, Sprite* target) override;
  void Move(const Point& move, Sprite* sprite) override;

 private:
  GameState game_state_;
};

}  // namespace babel

#endif  // BABEL_ENGINE_H__
