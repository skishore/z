#include "debug.h"
#include "SpriteState.h"

namespace skishore {

void SpriteState::Register(Sprite* sprite) {
  ASSERT(sprite != nullptr, "Registering NULL sprite!");
  ASSERT(self_ == nullptr, "Re-registering sprite!");
  self_ = sprite;
}

} // namespace skishore
