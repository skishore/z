
using namespace std;

#include "SDL2/SDL.h"
#include <algorithm>
#include <complex>
#include <map>

class Sprite {
    public:
        Sprite();
        ~Sprite();
        // load a sprite after screen is inited
        void loadSprite(const int, const int, const char*, const int, const int, \
                SDL_Surface* const, const SDL_Rect);
        // draw sprite to screen, erase sprite, and save ground under sprite
        void draw(SDL_Surface*);
        void erase(SDL_Surface*);
        void saveUnder(SDL_Surface*);
        // position and size
        float x, y;
        int width, height;
        // column and row of current sprite frame, and current anim counter
        int frameCol, frameRow, animNum;
    private:
        int numCols, numRows, depth;
        SDL_Surface *temp, *sprite;
        // drawing rects and positioning procedure
        SDL_Rect rSrc, rTarget, rBound;
        bool positionRects();
        // static sprite cache and reference count
        static int numSprites;
        static map<string, SDL_Surface*> cache;
};

int Sprite::numSprites = 0;
map<string, SDL_Surface*> Sprite::cache = map<string, SDL_Surface*>();

