#include <map>
#include <vector>

#include "ui/Tween.h"

using std::map;
using std::move;
using std::unique_ptr;
using std::vector;

namespace babel {
namespace ui {
namespace {

static const int kTweenFrames = 4;

class CameraEvent : public TweenEvent {
 public:
  CameraEvent(const Point& move) : move_(move) {}

  void Update(int frame, Transform* transform) override {
    transform->camera_offset = frame*kGridSize*move_/kTweenFrames;
  }

 private:
  Point move_;
};

class MoveEvent : public TweenEvent {
 public:
  MoveEvent(engine::sid id, const Point& move) : id_(id), move_(move) {}

  void Update(int frame, Transform* transform) override {
    transform->sprite_offsets[id_] = frame*kGridSize*move_/kTweenFrames;
  }

 private:
  engine::sid id_;
  Point move_;
};

}  // namespace

Tween::Tween(const engine::View& s, const engine::View& e) : start(s), end(e) {
  if (end.offset != start.offset) {
    AddEvent(new CameraEvent(end.offset - start.offset));
  }
  for (const auto& pair : start.sprites) {
    engine::sid id = pair.first;
    if (end.sprites.find(id) != end.sprites.end()) {
      const Point& prev = start.sprites.at(id).square + start.offset;
      const Point& next = end.sprites.at(id).square + end.offset;
      if (next != prev) {
        AddEvent(new MoveEvent(id, next - prev));
      }
    } else {
      transform.hidden_sprites.insert(id);
    }
  }
}

void Tween::AddEvent(TweenEvent* event) {
  events.push_back(unique_ptr<TweenEvent>(event));
}

bool Tween::Update() {
  frame += 1;
  if (frame > kTweenFrames || (events.empty() && frame > 1)) {
    return false;
  }
  for (auto& event : events) {
    event->Update(frame, &transform);
  }
  return true;
}

void Tween::Draw(Graphics* graphics) const {
  ASSERT(frame <= kTweenFrames, "Draw called after tween was finished!");
  if (frame < kTweenFrames && !events.empty()) {
    graphics->Draw(start, transform);
  } else {
    graphics->Draw(end);
  }
}

} // namespace ui 
} // namespace babel
