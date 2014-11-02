
using namespace std;

// global display constants
#define GAMEWIDTH 256
#define GAMEHEIGHT 256

#include "zSprite.cpp"
#include "Audio.h"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <cstring>
#include "sys/time.h"
extern "C" {
#include "SDL_prims.h"
}

// display constants
#define SPRITESIZE 16
#define ITEMSIZE 12
#define NUMCOLS (GAMEWIDTH/SPRITESIZE)
#define NUMROWS (GAMEHEIGHT/SPRITESIZE)
#define BITDEPTH 32

// size of the GUI and the screen
#define GUIHEIGHT 24
#define SCREENWIDTH GAMEWIDTH
#define SCREENHEIGHT (GAMEHEIGHT + GUIHEIGHT)

// the game world is a square of WORLDSIZE screens
#define WORLDSIDE 64
#define WORLDSIZE (WORLDSIDE*WORLDSIDE*NUMROWS*NUMCOLS)
#define ZONESIZE 3

// initial position
#define INITZONEX (WORLDSIDE*NUMCOLS/2)
#define INITZONEY (WORLDSIDE*NUMCOLS/2)
#define INITCAMERAX GAMEWIDTH
#define INITCAMERAY GAMEHEIGHT

// trap the player in a BOXSIZE square at the center of the screen
#define BOXSIZE 2*SPRITESIZE

// the fraction of a sprite's top, left, and right
// edges that can overlap an occupied square
#define TOLERANCE (0.2*SPRITESIZE)
#define PUSHAWAY (0.5*SPRITESIZE)

// game loop constants
#define FRAMERATE 60
#define TICKSPERSEC 1000000
#define FRAMEDELAY (TICKSPERSEC/FRAMERATE)
#define MINDELAY 1000
#define EVENTSPERFRAME 4

// length of day (MAXFRAMENUM) and rate of player stat attrition
#define MINUTESPERDAY 40
#define MAXFRAMENUM (MINUTESPERDAY*60*FRAMERATE)
const float DELTASTAT[NUMSTATS] = \
        {-1.0f*(MAXSTATS[SLP] - MAXSTATS[STA])/MAXFRAMENUM, \
        -0.8f*(MAXSTATS[EAT] - MAXSTATS[STA])/MAXFRAMENUM, \
        1.0f*MAXSTATS[STA]/(16*FRAMERATE)};
#define RUNSTA (-1.0f*MAXSTATS[STA]/(32*FRAMERATE))
#define PUNCHSTA -7.2f
#define PUNCHSTADIFF -2.4f
#define GRABSTA (-1.0f*MAXSTATS[STA]/(16*FRAMERATE))
#define LONGGRABSTA (-1.0f*MAXSTATS[STA]/(8*FRAMERATE))

// animation constants
#define ANIMFRAMES 8

// movement engine constants
#define PSPEED 0.95f
#define PRUNSPEED 1.4f
#define GUNSPEED 0.55f
#define GUNRUNSPEED 1.0f
#define ZSPEED 0.65f
#define ZRUNSPEED 1.1f

#define BULLETSPEED 16.8f
#define NUMBULLETSTEPS 4
#define SIGMA (BULLETSPEED/(2*NUMBULLETSTEPS))

// zombie kinematic constraint constants
#define SEPARATION 32
#define SENSITIVITY 9
#define PFORCE 1.1f
#define DIRECTIONALITY 0.4f
#define MINFORCEDIST 2.0f

// zombie probability distribution constants
#define MAXNUMZOMBIES 32
#define ZOMBIESTART 7

#define MAXNUMBULLETS 12

// game loop variables - when gameover is nonzero, exit
int gameover = 0;
int frameNum = 0;

// store player's key input in a list, ordered by time
list<int> moveDir;

// drawing surfaces and player sprite
SDL_Rect rcSrc, rcSprite;
SDL_Surface* screen;
SDL_Surface* buffer;
Sprite* tileset;
Player* player;
Sprite* item;

// zombie sprites and numbers
int numZombies = MAXNUMZOMBIES;
Zombie* zombies[MAXNUMZOMBIES];
list<zSprite*> allSprites;

// bullets and associated variables
int numBullets = MAXNUMBULLETS;
bullet bullets[MAXNUMBULLETS];

// store the entire world in RAM
unsigned char world[WORLDSIDE][WORLDSIDE][NUMCOLS][NUMROWS];
unsigned char tiles[ZONESIZE*NUMCOLS][ZONESIZE*NUMROWS];
point zone;
point camera;

point mouse;

#if !defined(__APPLE__) && !defined(WINDOWS)
int main(const int, const char**);
#else
int main(int, char**);
#endif
void loadZone();
void loadZoneFragment(const point, const point, const point, const point);
void drawTiles(const point);
void redrawTiles();
void drawMouse();

void gameLoop();
void HandleEvent(const SDL_Event);

void movePlayer();
void moveZombie(Zombie* const);
void avoidZombies(const zSprite* const, fpoint*, const int=SENSITIVITY);
void moveSprite(zSprite* const, fpoint*, const int=0);
void moveBullet(bullet* const);

void checkSquares(const zSprite* const, fpoint*);
bool checkSquare(const point);
bool checkSquare(const int, const int);

bool intersect(const Sprite* const, const Sprite* const);
bool intersectDir(const zSprite* const, const Sprite* const, const int=0);

void scrollCamera();
void cleanup();

// list utility functions
void addItemToList(list<int>* const, const int, const int=1);
bool contains(list<int>* const, const int);

