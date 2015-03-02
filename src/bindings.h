#ifndef __BABEL_BINDINGS_H__
#define __BABEL_BINDINGS_H__
#ifdef EMSCRIPTEN

#include <memory>

#include <emscripten/bind.h>

#include "base/point.h"
#include "engine/Action.h"
#include "engine/Engine.h"
#include "engine/View.h"

using namespace emscripten;

namespace babel {

// A list of basic types. WARNING: the vector types must be deleted!

EMSCRIPTEN_BINDINGS(stl_wrappers) {
  register_vector<std::string>("VectorString");
  register_vector<engine::TileView>("VectorTile");
  register_vector<std::vector<engine::TileView>>("VectorVectorTile");
  register_vector<engine::SpriteView>("VectorSprite");
};

EMSCRIPTEN_BINDINGS(value_types) {
  value_object<Point>("BabelPoint")
    .field("x", &Point::x)
    .field("y", &Point::y);

  value_object<engine::TileView>("BabelTile")
    .field("graphic", &engine::TileView::graphic)
    .field("visible", &engine::TileView::visible);

  value_object<engine::SpriteView>("BabelSprite")
    .field("id", &engine::SpriteView::id)
    .field("graphic", &engine::SpriteView::graphic)
    .field("color", &engine::SpriteView::color)
    .field("square", &engine::SpriteView::square);

  value_object<engine::StatusView>("BabelStatus")
    .field("cur_health", &engine::StatusView::cur_health)
    .field("max_health", &engine::StatusView::max_health);
};

// Actions go here. We need a helper method to construct each one.

inline engine::Action* MakeMoveAction(const Point point) {
  return new engine::MoveAction(point);
}

EMSCRIPTEN_BINDINGS(action) {
  class_<engine::Action>("BabelAction");
  function("MakeMoveAction", &MakeMoveAction, allow_raw_pointers());
};

EMSCRIPTEN_BINDINGS(engine_view) {
  class_<engine::Engine>("BabelEngine")
    .constructor<>()
    .function("AddEventHandler", &engine::Engine::AddEventHandler,
              allow_raw_pointers())
    .function("AddInput", &engine::Engine::AddInput, allow_raw_pointers())
    .function("GetView", &engine::Engine::GetView, allow_raw_pointers())
    .function("Update", &engine::Engine::Update);

  class_<engine::View>("BabelView")
    .property("offset", &engine::View::offset)
    .property("tiles", &engine::View::tiles)
    .property("sprites", &engine::View::sprites)
    .property("log", &engine::View::log)
    .property("status", &engine::View::status);
};

// Animated events go here. We need to add each handler function to the wrapper.

struct EventHandlerWrapper : public wrapper<engine::EventHandler> {
  EMSCRIPTEN_WRAPPER(EventHandlerWrapper);
  void BeforeAttack(engine::sid source, engine::sid target) override {
    return call<void>("BeforeAttack", source, target);
  }
  void LaunchDialog(const Point& source) override {
    return call<void>("LaunchDialog", source);
  }
};

EMSCRIPTEN_BINDINGS(events) {
  class_<engine::EventHandler>("BabelEventHandler")
    .function("BeforeAttack", &engine::EventHandler::BeforeAttack,
              pure_virtual())
    .function("LaunchDialog", &engine::EventHandler::LaunchDialog,
              pure_virtual())
    .allow_subclass<EventHandlerWrapper>("EventHandlerWrapper");
};

}  // namespace babel

#endif  // EMSCRIPTEN
#endif  // __BABEL_BINDINGS_H__
