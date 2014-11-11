#include <iostream>
#include <SDL2/SDL_ttf.h>

#include "Engine.h"
#include "Point.h"

using std::cout;
using std::endl;
using skishore::Engine;
using skishore::Point;

namespace {
static const Point kScreenSize(16, 16);
static const int kFrameRate = 60;
}  // namespace

int main(int argc, char** argv) {
  TTF_Font* font;
  font = TTF_OpenFont("font.ttf", 16);
  if (!font) {
    cout << "TTF_OpenFont error: " << TTF_GetError() << endl;
    return -1;
  } else {
    cout << "What? Got a font.." << endl;
    return -1;
  }

  Engine(kFrameRate, kScreenSize);
  return 0;
}
