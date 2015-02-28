#ifndef __BABEL_ENGINE_ENGINE_H__
#define __BABEL_ENGINE_ENGINE_H__

#include <deque>
#include <memory>

#include "base/point.h"
#include "engine/Action.h"
#include "engine/EventHandler.h"
#include "engine/GameState.h"
#include "engine/Sprite.h"
#include "engine/View.h"
#include "interface/Dialog.h"

namespace babel {
namespace engine {

class Engine {
 public:
  Engine();
  ~Engine();

  // Does NOT take ownership of the input EventHandler.
  void AddEventHandler(EventHandler* handler);

  // Takes ownership of the input Action.
  void AddInput(Action* input);

  // Runs a single update step. Returns true if the graphics need to be
  // redrawn because something changed.
  bool Update();

  // Returns a non-null dialog if this game is blocking on input.
  interface::Dialog* GetDialog() const;

  // The caller takes ownership of the new view.
  const View* GetView(int radius) const;

 private:
  GameState game_state_;
  DelegatingEventHandler handler_;
  std::deque<Action*> inputs_;
  std::unique_ptr<Action> interrupt_;
};

}  // namespace engine
}  // namespace babel

#endif  // __BABEL_ENGINE_ENGINE_H__
