#ifndef __SKISHORE_ENGINE_H__
#define __SKISHORE_ENGINE_H__

#include "GameLoop.h"
#include "InputHandler.h"
#include "Point.h"
#include "ScrollingGraphics.h"
#include "TileMap.h"

namespace skishore {

class Engine : public GameLoop::Updatable {
 public:
  // Automatically starts when constructed.
  Engine(int frame_rate, const Point& screen_size);

  bool Update(double frame_rate);

 private:
  const Point screen_size_;
  InputHandler input_;
  TileMap map_;
  std::unique_ptr<ScrollingGraphics> graphics_;
};

} // namespace skishore

#endif  // __SKISHORE_ENGINE_H__
