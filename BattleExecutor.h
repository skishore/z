#ifndef __SKISHORE_BATTLE_EXECUTOR_H__
#define __SKISHORE_BATTLE_EXECUTOR_H__

#include <memory>
#include <string>

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
  BattleScript* AssumePlace(int i);
  BattleScript* AssumePlaces();
  BattleScript* Attack(int i, int target);
  BattleScript* Freeze();
  BattleScript* MoveNextTo(int i, int target);
  BattleScript* Speak(int i, const std::string& text);

  // Takes ownership and runs the given script. This method should not be
  // called if a script is already running.
  void RunScript(BattleScript* script);

  // Runs the current script through a single time-step. Returns true if
  // the script is finished.
  bool Update();

 private:
  const TileMap::Room& room_;
  const std::vector<Sprite*>& sprites_;

  Point center_;
  std::vector<Point> places_;
  std::unique_ptr<BattleScript> script_;
};

}  // namespace battle
}  // namespace skishore

#endif  // __SKISHORE_BATTLE_EXECUTOR_H__
