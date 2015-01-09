#ifndef __BABEL_GAME_LOOP_H__
#define __BABEL_GAME_LOOP_H__

namespace babel {
namespace ui {

class GameLoop {
 public:
  class Updatable;

  // Automatically starts when run.
  // Note that only one game loop may be running at a time.
  // Starting a second loop will lead to undefined behavior.
  GameLoop(int frame_rate, GameLoop::Updatable* engine);

  class Updatable {
   public:
    // This method should return false if it is time to exit the loop.
    virtual bool Update(double frame_rate) = 0;
  };
};

} // namespace ui
} // namespace babel

#endif  // __BABEL_GAME_LOOP_H__
