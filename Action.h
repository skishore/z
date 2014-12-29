#ifndef BABEL_ACTION_H__
#define BABEL_ACTION_H__

#include "Point.h"

namespace babel {

class GameState;
class Sprite;

class Action {
 public:
  void Bind(Sprite* sprite);
  virtual void Execute(GameState* game_state) = 0;

 protected:
  Sprite* sprite_ = nullptr;
};

class AttackAction : public Action {
 public:
  AttackAction(Sprite* target);
  void Execute(GameState* game_state) override;

 private:
  Sprite* target_;
};

class MoveAction : public Action {
 public:
  MoveAction(const Point& move);
  void Execute(GameState* game_state) override;

 private:
  Point move_;
};

}  // namespace babel

#endif  // BABEL_ACTION_H__
