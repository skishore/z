#ifndef BABEL_ACTION_H__
#define BABEL_ACTION_H__

#include "base/point.h"

namespace babel {
namespace engine {

class GameState;
class Sprite;

class Action {
 public:
  void Bind(Sprite* sprite);

  // When an action is executed, it may set success = true, in which case the
  // sprite actually used its turn. It may also return an alternate action.
  // For example, if one sprite tries moving onto another sprite's square, the
  // move will not succeed, but it will return an attack as an alternate.
  virtual Action* Execute(GameState* game_state, bool* success) = 0;

 protected:
  Sprite* sprite_ = nullptr;
};

class AttackAction : public Action {
 public:
  AttackAction(Sprite* target);
  Action* Execute(GameState* game_state, bool* success) override;

 private:
  Sprite* target_;
};

class MoveAction : public Action {
 public:
  MoveAction(const Point& move);
  Action* Execute(GameState* game_state, bool* success) override;

 private:
  Point move_;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_ACTION_H__
