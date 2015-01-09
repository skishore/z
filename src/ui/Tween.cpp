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

static const int kGridSize = 32;
static const int kTweenFrames = 4;

class CameraEvent : public TweenEvent {
 public:
  CameraEvent(const Point& move) : move_(move) {}

  void Update(Tween* tween) override {
    tween->camera_offset = tween->frame*kGridSize*move_/kTweenFrames;
  }

 private:
  Point move_;
};

class MoveEvent : public TweenEvent {
 public:
  MoveEvent(engine::sid id, const Point& move) : id_(id), move_(move) {}

  void Update(Tween* tween) override {
    tween->sprite_offsets[id_] = tween->frame*kGridSize*move_/kTweenFrames;
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
      sprite_offsets[id] = Point(0, 0);
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
    event->Update(this);
  }
  return true;
}

void Tween::Draw(Graphics* graphics) const {
  ASSERT(frame <= kTweenFrames, "Draw called after tween was finished!");
  graphics->Clear();
  if (frame < kTweenFrames && !events.empty()) {
    graphics->DrawTiles(start, camera_offset);
    for (const auto& pair : sprite_offsets) {
      const engine::SpriteView& sprite = start.sprites.at(pair.first);
      graphics->DrawSprite(sprite, pair.second - camera_offset);
    }
  } else {
    graphics->Draw(end);
  }
  graphics->Flip();
}

} // namespace ui 
} // namespace babel
