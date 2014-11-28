#include <algorithm>
#include "hungarian.h"

#include "constants.h"
#include "debug.h"
#include "BattleState.h"
#include "BattleExecutor.h"

using std::min;
using std::max;
using std::vector;

namespace skishore {
namespace battle {

namespace {

// Takes an integer and a divisor and does division and rounding.
inline int divround(int a, int b) {
    return (a + (a > 0 ? b/2 : -b/2))/b;
}

// Takes a position and returns the square on the perimeter of the given room
// that is closest to that position.
Point ClosestPerimeterSquare(const Point& position, const TileMap::Room& room) {
  Point result = kGridTicks*room.position;
  if (position.x > result.x + kGridTicks*room.size.x/2) {
    result.x += kGridTicks*room.size.x;
  }
  if (position.y > result.y + kGridTicks*room.size.y/2) {
    result.y += kGridTicks*room.size.y;
  }
  if (abs(result.x - position.x) > abs(result.y - position.y)) {
    result.x = position.x;
  } else {
    result.y = position.y;
  }
  result.x = divround(result.x, kGridTicks);
  result.y = divround(result.y, kGridTicks);
  Point other_corner = room.position + room.size - Point(1, 1);
  result.x = min(max(room.position.x, result.x), other_corner.x);
  result.y = min(max(room.position.y, result.y), other_corner.y);
  return result;
}

// Advance distance steps along the room's perimeter in the ccw direction.
Point AdvanceAlongPerimeter(
    const Point& square, const TileMap::Room& room, int distance) {
  if (room.size.x <= 1 || room.size.y <= 1) {
    return square;
  }
  Point result = square;
  while (distance > 0) {
    if (result.x == room.position.x) {
      int new_y = max(result.y - distance, room.position.y);
      distance -= result.y - new_y;
      result.y = new_y;
    } else if (result.x == room.position.x + room.size.x - 1) {
      int new_y = min(result.y + distance, room.position.y + room.size.y - 1);
      distance -= new_y - result.y;
      result.y = new_y;
    }
    if (result.y == room.position.y) {
      int new_x = min(result.x + distance, room.position.x + room.size.x - 1);
      distance -= new_x - result.x;
      result.x = new_x;
    } else if (result.y == room.position.y + room.size.y - 1) {
      int new_x = max(result.x - distance, room.position.x);
      distance -= result.x - new_x;
      result.x = new_x;
    }
  }
  return result;
}

void ComputePlaces(const TileMap::Room& room, const vector<Sprite*>& sprites,
                   vector<Point>* places) {
  ASSERT(sprites.size() > 0, "Got an empty sprites list!");
  const int n = sprites.size();
  vector<Point> options(n);
  places->resize(n);

  Point half_square(kGridTicks/2, kGridTicks/2);
  Point position = sprites[0]->GetPosition() + half_square;
  options[0] = ClosestPerimeterSquare(position, room);

  const int perimeter = 2*(room.size.x + room.size.y) - 4;
  for (int i = 1; i < n; i++) {
    const int distance = i*perimeter/n - (i - 1)*perimeter/n;
    options[i] = AdvanceAlongPerimeter(options[i - 1], room, distance);
  }

  vector<vector<Cost>> distances(n, vector<Cost>(n));
  for (int i = 0; i < n; i++) {
    Point position = sprites[i]->GetPosition() + half_square;
    for (int j = 0; j < n; j++) {
      double distance = (position - kGridTicks*options[j]).length();
      distances[i][j] = (int)distance;
    }
  }
  Hungarian hungarian(n, distances);
  Hungarian::Status status = hungarian.Solve();
  ASSERT(status == Hungarian::OK, "Hungarian exited with status: " << status);

  for (int i = 0; i < n; i++) {
    (*places)[i] = options[hungarian.GetXMatch(i)];
  }
}

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
