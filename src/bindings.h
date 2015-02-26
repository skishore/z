#ifndef __BABEL_BINDINGS_H__
#define __BABEL_BINDINGS_H__
#ifdef EMSCRIPTEN

#include <memory>

#include <emscripten/bind.h>

#include "base/point.h"
#include "engine/Engine.h"
#include "engine/View.h"

using namespace emscripten;

namespace babel {

std::unique_ptr<engine::Engine> gEngine;

const engine::View* GetView() {
  return new engine::View(3, gEngine->GetGameState());
}

EMSCRIPTEN_BINDINGS(stl_wrappers) {
  register_vector<std::string>("StringVector");
  register_vector<engine::TileView>("TileVector");
  register_vector<std::vector<engine::TileView>>("2DTileVector");
  register_map<engine::sid,engine::SpriteView>("SpriteMap");
};

EMSCRIPTEN_BINDINGS(value_types) {
  value_object<Point>("BabelPoint")
    .field("x", &Point::x)
    .field("y", &Point::y);

  value_object<engine::TileView>("BabelTile")
    .field("graphic", &engine::TileView::graphic)
    .field("visible", &engine::TileView::visible);

  value_object<engine::SpriteView>("BabelSprite")
    .field("graphic", &engine::SpriteView::graphic)
    .field("color", &engine::SpriteView::color)
    .field("square", &engine::SpriteView::square);

  value_object<engine::StatusView>("StatusView")
    .field("cur_health", &engine::StatusView::cur_health)
    .field("max_health", &engine::StatusView::max_health);

  class_<engine::View>("View")
    .property("size", &engine::View::size)
    .property("tiles", &engine::View::tiles)
    .property("sprites", &engine::View::sprites)
    .property("log", &engine::View::log)
    .property("status", &engine::View::status);
};

EMSCRIPTEN_BINDINGS(view) {
  function("BabelGetView", &GetView, allow_raw_pointers());
}

}  // namespace babel

#endif  // EMSCRIPTEN
#endif  // __BABEL_BINDINGS_H__
