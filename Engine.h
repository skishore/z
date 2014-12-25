#ifndef BABEL_ENGINE_H__
#define BABEL_ENGINE_H__

#include "constants.h"
#include "Point.h"
#include "TileMap.h"
#include "View.h"

namespace babel {

class Engine {
 public:
  Engine();

  // Caller takes ownership of the view.
  const View* GetView() const;

  // Returns true if the command was valid.
  // If this method returns false, the view does not need to be redrawn.
  bool HandleCommand(char command);

 private:
  TileMap tiles_;
  Point player_position_;
  std::vector<Point> enemy_positions_;
};

}  // namespace babel

#endif  // BABEL_ENGINE_H__
