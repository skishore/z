#include "ui/Tween.h"

#include <map>
#include <vector>

using std::map;
using std::move;
using std::unique_ptr;
using std::vector;

namespace babel {
namespace ui {
namespace {

static const int kTweenFrames = 4;
static const int kGridSize = 16;

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
  map<engine::sid,const engine::SpriteView*> end_sprites;
  for (const auto& sprite : end.sprites) {
    end_sprites[sprite.id] = &sprite;
  }
  for (const auto& sprite : start.sprites) {
    const engine::sid id = sprite.id;
    if (end_sprites.find(id) != end_sprites.end()) {
      const Point& prev = sprite.square + start.offset;
      const Point& next = end_sprites.at(id)->square + end.offset;
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

void Tween::Draw(const engine::View** view, const Transform** t) const {
  ASSERT(frame <= kTweenFrames, "Draw called after tween was finished!");
  if (frame < kTweenFrames && !events.empty()) {
    *view = &start;
    *t = &transform;
    return;
  }
  *view = &end;
}

} // namespace ui
} // namespace babel
