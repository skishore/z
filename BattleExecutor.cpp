#include <algorithm>
#include "hungarian.h"

#include "constants.h"
#include "debug.h"
#include "BattleData.h"
#include "BattleExecutor.h"
#include "BattleState.h"

using std::min;
using std::max;
using std::vector;

namespace skishore {
namespace battle {

namespace {
namespace script {

class ParallelScript : public BattleScript {
 public:
  ParallelScript(BattleScript* script1, BattleScript* script2)
      : script1_(script1), script2_(script2) {}

  bool Step() override {
    bool script1_done = script1_->Update();
    bool script2_done = script2_->Update();
    return script1_done && script2_done;
  }

 private:
  std::unique_ptr<BattleScript> script1_;
  std::unique_ptr<BattleScript> script2_;
};

class SerialScript : public BattleScript {
 public:
  SerialScript(BattleScript* script1, BattleScript* script2)
      : script1_(script1), script2_(script2) {}

  bool Step() override {
    return script1_->Update() && script2_->Update();
  }

 private:
  std::unique_ptr<BattleScript> script1_;
  std::unique_ptr<BattleScript> script2_;
};

class WaitForStateTransitionScript : public BattleScript {
 public:
  // Takes ownership of the input state.
  WaitForStateTransitionScript(Sprite* sprite, SpriteState* state)
      : sprite_(sprite), state_(state), state_ownership_(state) {}

  void Start() override {
    sprite_->SetState(state_ownership_.release());
  }

  bool Step() override {
    return sprite_->GetState() != state_;
  }

 private:
  Sprite* sprite_;
  SpriteState* state_;
  std::unique_ptr<SpriteState> state_ownership_;
};

}  // namespace script

inline BattleScript* Run(Sprite* sprite, SpriteState* state) {
  return new script::WaitForStateTransitionScript(sprite, state);
}

void ComputePlaces(const TileMap::Room& room, const vector<Sprite*>& sprites,
                   vector<Point>* places) {
  ASSERT(sprites.size() > 0, "Got an empty sprites list!");
  const int n = sprites.size();
  places->resize(n);
  vector<Point> options(n - 1);

  Point half_square(kGridTicks/2, kGridTicks/2);
  (*places)[0] = Point(room.position.x + room.size.x - 1,
                       room.position.y + room.size.y/2);

  for (int i = 0; i < n - 1; i++) {
    const int offset = (i == 0 || i == n - 2 ? 3 : 1);
    options[i] = Point(room.position.x + offset, room.position.y + 2*i);
  }

  vector<vector<Cost>> distances(n - 1, vector<Cost>(n - 1));
  for (int i = 0; i < n - 1; i++) {
    Point position = sprites[i + 1]->GetPosition() + half_square;
    for (int j = 0; j < n - 1; j++) {
      double distance = (position - kGridTicks*options[j]).length();
      distances[i][j] = (int)distance;
    }
  }
  Hungarian hungarian(n - 1, distances);
  Hungarian::Status status = hungarian.Solve();
  ASSERT(status == Hungarian::OK, "Hungarian exited with status: " << status);

  for (int i = 0; i < n - 1; i++) {
    (*places)[i + 1] = options[hungarian.GetXMatch(i)];
  }
}

}  // namespace

BattleScript* BattleScript::And(BattleScript* other) {
  return new script::ParallelScript(this, other);
}

BattleScript* BattleScript::AndThen(BattleScript* other) {
  return new script::SerialScript(this, other);
}

bool BattleScript::Update() {
  if (!started_) {
    Start();
    started_ = true;
  }
  done_ = done_ || Step();
  return done_;
}

BattleExecutor::BattleExecutor(
    const TileMap::Room& room, const vector<Sprite*>& sprites)
    : room_(room), sprites_(sprites) {
  ComputePlaces(room_, sprites_, &places_);
  center_ = kGridTicks*(2*room.position + room.size - Point(1, 1))/2;
  for (Sprite* sprite : sprites_) {
    sprite->battle_.reset(new BattleData);
    if (!sprite->is_player_) {
      sprite->battle_->text = "excellent";
      sprite->battle_->dir = Direction::LEFT;
    }
  }
}

BattleScript* BattleExecutor::AssumePlace(int i) {
  BattleScript* move = Run(sprites_[i], new WalkToTargetState(places_[i]));
  const Point& target = (i == 0 ? center_ : kGridTicks*places_[0]);
  BattleScript* face = Run(sprites_[i], new FaceTargetState(target));
  return move->AndThen(face);
}

BattleScript* BattleExecutor::AssumePlaces() {
  BattleScript* script = AssumePlace(0);
  for (int i = 1; i < sprites_.size(); i++) {
    script = script->And(AssumePlace(i));
  }
  return script;
}

void BattleExecutor::RunScript(BattleScript* script) {
  ASSERT(script_ == nullptr, "RunScript called when one was running!");
  script_.reset(script);
}

bool BattleExecutor::Update() {
  if (script_ != nullptr && script_->Update()) {
    script_.reset(nullptr);
  }
  return script_ != nullptr;
}

}  // namespace battle
}  // namespace skishore
