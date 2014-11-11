#ifndef __SKISHORE_GAME_LOOP_H__
#define __SKISHORE_GAME_LOOP_H__

namespace skishore {

class GameLoop {
 public:
  class Updatable;

  // Automatically starts when run.
  // Note that only one game loop may be running at a time.
  // Starting a second loop will lead to undefined behavior.
  GameLoop(int frame_rate, GameLoop::Updatable* engine);

  class Updatable {
   public:
    // This method should return true if it is time to exit the loop.
    virtual bool Update() = 0;
  };
};

} // namespace skishore

#endif  // __SKISHORE_GAME_LOOP_H__
