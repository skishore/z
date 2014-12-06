#ifndef __SKISHORE_BATTLE_EXECUTOR_H__
#define __SKISHORE_BATTLE_EXECUTOR_H__

#include <memory>

#include "Sprite.h"
#include "TileMap.h"

namespace skishore {
namespace battle {

class BattleScript {
 public:
  BattleScript* And(BattleScript* other);
  BattleScript* AndThen(BattleScript* other);
  bool Update();

  // Start will be called once when the script is started.
  // Step will be called once per frame until the script is done,
  // at which point it should return true.
  virtual void Start() {};
  virtual bool Step() = 0;

 private:
  bool started_ = false;
  bool done_ = false;
};

class BattleExecutor {
 public:
  BattleExecutor(const TileMap::Room& room,
                 const std::vector<Sprite*>& sprites);

  const Point& GetCenter() const { return center_; }

  // Primitive scripts that can be chained to create larger ones.
  // AssumePlace moves sprite i to its place in the battle,
  // while AssumePlaces moves all sprites to their places.
  BattleScript* AssumePlace(int i);
  BattleScript* AssumePlaces();

  // Takes ownership and runs the given script. This method should not be
  // called if a script is already running.
  void RunScript(BattleScript* script);

  // Runs the current script through a single time-step. Returns true if
  // the script is finished.
  bool Update();

 protected:
  const TileMap::Room& room_;
  const std::vector<Sprite*>& sprites_;

  Point center_;
  std::vector<Point> places_;
  std::unique_ptr<BattleScript> script_;
};

}  // namespace battle
}  // namespace skishore

#endif  // __SKISHORE_BATTLE_EXECUTOR_H__
