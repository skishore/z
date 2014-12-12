#include <algorithm>
#include "hungarian.h"

#include "constants.h"
#include "debug.h"
#include "BattleData.h"
#include "BattleExecutor.h"
#include "BattleState.h"

using std::min;
using std::max;
using std::string;
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

class SetStateScript : public WaitForStateTransitionScript {
 public:
  using WaitForStateTransitionScript::WaitForStateTransitionScript;
  bool Step() override { return true; }
};

}  // namespace script

inline BattleScript* Run(Sprite* sprite, SpriteState* state) {
  return new script::WaitForStateTransitionScript(sprite, state);
}

inline BattleScript* Set(Sprite* sprite, SpriteState* state) {
  return new script::SetStateScript(sprite, state);
}

const static int kMinSpacing = kGridTicks;

void ComputeSemicircle(
    const TileMap::Room& room, const Point& center, int height,
    int n, int offset, int sign, int num_circles, vector<Point>* places) {
  const int spacing = max((height - n*kGridTicks)/(n + 1), kMinSpacing);
  const int first_space = (height - n*kGridTicks - (n - 1)*spacing)/2;
  int y = kGridTicks*room.position.y + first_space;
  for (int i = 0; i < n; i++) {
    const bool can_shift = (n > 2 || num_circles == 1);
    const bool shift_in = can_shift && (i == 0 || i == n - 1);
    const int shift = min(room.size.x - 1, (shift_in ? 2 : 4));
    (*places)[i + offset].x = center.x + (sign*shift - 1)*kGridTicks/2;
    (*places)[i + offset].y = y;
    y += kGridTicks + spacing;
  }
}

void ComputePlaces(const TileMap::Room& room, const Point& center,
                   const vector<Sprite*>& sprites, vector<Point>* places) {
  ASSERT(sprites.size() > 0, "Got an empty sprites list!");
  const int n = sprites.size();
  places->resize(n);
  vector<Point> options(n - 1);

  const int height = kGridTicks*room.size.y;
  const int max_per_side = (height - kGridTicks)/(kGridTicks + kMinSpacing) + 1;
  const int sign =
      (sprites[0]->GetPosition().x + kGridTicks/2 < center.x ? 1 : -1);

  if (n - 1 <= max_per_side) {
    (*places)[0].x = center.x + (-sign*min(room.size.x - 1, 2) - 1)*kGridTicks/2;
    (*places)[0].y = center.y - kGridTicks/2;
    ComputeSemicircle(room, center, height, n - 1, 0, sign, 1, &options);
  } else {
    (*places)[0].x = center.x - kGridTicks/2;
    (*places)[0].y = center.y - kGridTicks/2;
    ComputeSemicircle(room, center, height, n/2, 0, sign, 2, &options);
    ComputeSemicircle(room, center, height, (n - 1)/2, n/2, -sign, 2, &options);
  }

  vector<vector<Cost>> distances(n - 1, vector<Cost>(n - 1));
  for (int i = 0; i < n - 1; i++) {
    const Point& position = sprites[i + 1]->GetPosition();
    for (int j = 0; j < n - 1; j++) {
      distances[i][j] = (int)(position - options[j]).length();
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
  center_ = kGridTicks*(2*room.position + room.size)/2;
  ComputePlaces(room_, center_, sprites_, &places_);
  for (int i = 1; i < sprites.size(); i++) {
    Sprite* sprite = sprites[i];
    sprite->battle_.reset(new BattleData);
    if (!sprite->is_player_) {
      sprite->battle_->side =
          (places_[i].x < places_[0].x ? Direction::LEFT : Direction::RIGHT);
    }
  }
}

BattleScript* BattleExecutor::AssumePlace(int i) {
  BattleScript* move = Run(sprites_[i], new WalkToTargetState(places_[i]));
  const Point target =
      (i == 0 ? center_ - Point(kGridTicks/2, kGridTicks/2) : places_[0]);
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

BattleScript* BattleExecutor::Freeze() {
  BattleScript* script = Set(sprites_[0], new WaitingState);
  for (int i = 1; i < sprites_.size(); i++) {
    script = script->And(Set(sprites_[i], new WaitingState));
  }
  return script;
}

BattleScript* BattleExecutor::Speak(int i, const string& text) {
  return Run(sprites_[i], new SpeakState(Direction::UP, text));
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
