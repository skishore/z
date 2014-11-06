#include "Sprite.h"

using std::string;

namespace skishore {

Sprite::Sprite(const Point& size, ImageCache* cache) :
    size_(size), cache_(cache), image_(nullptr) {}

Sprite::~Sprite() {
  cache_->FreeImage(image_);
}

bool Sprite::LoadImage(const string& filename) {
  cache_->FreeImage(image_);
  return cache_->LoadImage(filename, &image_);
}

}  // namespace skishore

/*
void Sprite::draw(SDL_Surface* target) {
    if (positionRects() == true) {
        rSrc.x = rSrc.x + width*frameCol;
        rSrc.y = rSrc.y + height*frameRow;
        SDL_BlitSurface(sprite, &rSrc, target, &rTarget);
    }
}

bool Sprite::positionRects() {
    int x = round(this->x);
    int y = round(this->y);

    // don't draw if sprite is outside screen
    if ((x + width < 0) or (x > rBound.w) or (y + height < 0) or (y > rBound.h)) {
        return false;
    } else {
        // place destination rect, taking care around boundary
        rTarget.x = max(x, 0);
        rTarget.y = max(y, 0);
        rTarget.w = min(width + min(x, 0), rBound.w - x);
        rTarget.h = min(height + min(y, 0), rBound.h - y);

        // place source rect
        rSrc.x = rTarget.x - x;
        rSrc.y = rTarget.y - y;
        rSrc.w = rTarget.w;
        rSrc.h = rTarget.h;

        // move target into the bounding box
        rTarget.x += rBound.x;
        rTarget.y += rBound.y;

        return true;
    }
}

*/
