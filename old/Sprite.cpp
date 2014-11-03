
using namespace std;

#include "Sprite.h"

#include <iostream>

Sprite::Sprite() {
    numSprites++;    
}

void Sprite::loadSprite(const int a, const int b, const char* name, const int cols, const int rows, \
        SDL_Surface* const screen, const SDL_Rect bound) {
    int colorKey;
    string sname;
    char dirname[32];

    width = a;
    height = b;
    x = 0;
    y = 0;
    frameCol = 0;
    frameRow = 0;
    animNum = 0;
    numCols = cols;
    numRows = rows;
    rBound = SDL_Rect(bound);
   
    // if the sprite image is cached, use the cache
    sname = string(name);
    if (cache.count(sname) > 0) {
        sprite = cache[sname];
        return;
    }

    // load sprite
    sprintf(dirname, "Images/%s", name);
    SDL_Surface* temp = SDL_LoadBMP(dirname);
    sprite = SDL_ConvertSurface(temp, screen->format, 0);
    //assert(sprite->format->format == screen->format->format, "Format mismatch!");
    SDL_FreeSurface(temp);

    // set sprite transparent color
    //colorKey = SDL_MapRGB(screen->format, 255, 0, 255);
    //SDL_SetColorKey(sprite, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorKey);
    //SDL_SetAlpha(sprite, 0, 0);

    cache[sname] = sprite;
}

void Sprite::saveUnder(SDL_Surface* target) {
    if (positionRects() == true) {
        rSrc.x = rSrc.x + width*numCols;
        SDL_BlitSurface(target, &rTarget, sprite, &rSrc);
    }
}

void Sprite::erase(SDL_Surface* target) {
    if (positionRects() == true) {
        rSrc.x = rSrc.x + width*numCols;
        SDL_BlitSurface(sprite, &rSrc, target, &rTarget);
    }
}

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

Sprite::~Sprite() {
    numSprites--;
    if (numSprites == 0) {
        map<string, SDL_Surface*>::iterator it;
        for (it = cache.begin(); it != cache.end(); it++)
            SDL_FreeSurface((*it).second);
        cache.clear();
    }
}

