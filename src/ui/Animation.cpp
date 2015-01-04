#include <map>
#include <vector>

#include "base/debug.h"
#include "ui/Animation.h"

using std::map;
using std::move;
using std::unique_ptr;
using std::vector;

namespace babel {
namespace ui {

namespace {
static const int kGridSize = 16;
static const int kTweenFrames = 3;
}  // namespace

class Event {
 public:
  virtual void Update(Tween* tween) = 0;
};

class Tween {
 public:
  void AddEvent(Event* event) {
    events.push_back(unique_ptr<Event>(event));
  }

  bool Update() {
    frame += 1;
    if (frame == kTweenFrames || events.empty()) {
      return true;
    }
    for (auto& event : events) {
      event->Update(this);
    }
    return false;
  }

  void Draw(const engine::View& view, Graphics* graphics) {
    graphics->Clear();
    graphics->DrawTiles(view, camera_offset);
    for (const auto& pair : sprite_offsets) {
      const engine::SpriteView& sprite = view.sprites.at(pair.first);
      graphics->DrawSprite(sprite, pair.second - camera_offset);
    }
    graphics->Flip();
  }

  int frame = 0;
  vector<unique_ptr<Event>> events;
  Point camera_offset;
  map<engine::sid,Point> sprite_offsets;
};

class CameraEvent : public Event {
 public:
  CameraEvent(const Point& move) : move_(move) {}

  void Update(Tween* tween) override {
    tween->camera_offset = tween->frame*kGridSize*move_/kTweenFrames;
  }

 private:
  Point move_;
};

class MoveEvent : public Event {
 public:
  MoveEvent(engine::sid id, const Point& move) : id_(id), move_(move) {}

  void Update(Tween* tween) override {
    tween->sprite_offsets[id_] =
        tween->frame*kGridSize*move_/kTweenFrames;
  }

 private:
  engine::sid id_;
  Point move_;
};

Animation::~Animation() {
  if (tween_ != nullptr) {
    delete tween_;
  }
}

void Animation::HandleAttack(
    const engine::Sprite& source, const engine::Sprite& target) {}

void Animation::HandleMove(
    const engine::Sprite& sprite, const Point& square) {}

void Animation::SetNextView(engine::View* view) {
  ASSERT(next_ == nullptr, "SetNextView called with an animation running!");
  next_.reset(view);
  if (last_ != nullptr) {
    Reset();
  }
}

bool Animation::Update() {
  if (next_ != nullptr) {
    if (tween_ != nullptr && !tween_->Update()) {
      tween_->Draw(*last_, &graphics_);
    } else {
      Commit();
    }
  }
  return next_ == nullptr;
}

void Animation::Commit() {
  last_.reset(next_.release());
  graphics_.Clear();
  graphics_.Draw(*last_);
  graphics_.Flip();
}

void Animation::Reset() {
  if (tween_ != nullptr) {
    delete tween_;
  }
  tween_ = new Tween;

  if (next_->offset != last_->offset) {
    tween_->AddEvent(new CameraEvent(next_->offset - last_->offset));
  }
  for (const auto& pair : last_->sprites) {
    engine::sid id = pair.first;
    if (next_->sprites.find(id) != next_->sprites.end()) {
      const Point& last = last_->sprites[id].square + last_->offset;
      const Point& next = next_->sprites[id].square + next_->offset;
      if (next != last) {
        tween_->AddEvent(new MoveEvent(id, next - last));
      }
      tween_->sprite_offsets[id] = Point(0, 0);
    }
  }
}

} // namespace ui 
} // namespace babel
