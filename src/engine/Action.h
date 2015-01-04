#ifndef BABEL_ACTION_H__
#define BABEL_ACTION_H__

#include "base/point.h"

namespace babel {
namespace engine {

class Action;
class EventHandler;
class GameState;
class Sprite;

struct ActionResult {
  bool success = false;
  Action* alternate = nullptr;
};

class Action {
 public:
  // Sprite and GameState must not be null. The event handler may be null.
  void Bind(Sprite* sprite, GameState* game_state, EventHandler* handler);
  virtual ActionResult Execute() = 0;

 protected:
  Sprite* sprite_ = nullptr;
  GameState* game_state_ = nullptr;
  EventHandler* handler_ = nullptr;
};

class AttackAction : public Action {
 public:
  AttackAction(Sprite* target);
  ActionResult Execute() override;

 private:
  Sprite* target_;
};

class MoveAction : public Action {
 public:
  MoveAction(const Point& move);
  ActionResult Execute() override;

 private:
  Point move_;
};

}  // namespace engine
}  // namespace babel

#endif  // BABEL_ACTION_H__
