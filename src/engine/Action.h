#ifndef BABEL_ACTION_H__
#define BABEL_ACTION_H__

#include <memory>
#include <string>
#include <vector>

#include "base/point.h"
#include "interface/Dialog.h"

namespace babel {
namespace engine {

class Action;
class EventHandler;
class GameState;
class Sprite;

struct ActionResult {
  bool success = false;
  bool stalled = false;
  Action* alternate = nullptr;
};

class Action {
 public:
  virtual ~Action() {};

  // None of the input arguments may be null.
  void Bind(Sprite* sprite, GameState* game_state,
            std::vector<EventHandler*>* handlers);
  virtual ActionResult Execute() = 0;

  std::unique_ptr<interface::Dialog> dialog_;

 protected:
  Sprite* sprite_ = nullptr;
  GameState* game_state_ = nullptr;
  std::vector<EventHandler*>* handlers_ = nullptr;
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
