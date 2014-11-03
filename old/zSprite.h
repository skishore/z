
#include "Sprite.cpp"
#include "point.h"

#include <queue>

// player stats - slp, eat, and sta
#define NUMSTATS 3
#define SLP 0
#define EAT 1
#define STA 2
const int MAXSTATS[NUMSTATS] = {96, 88, 64};

#define MAXHEALTH 17

// player equipment constants
#define NONE 0
#define KNIFE 1
#define GUN 2

// player aiming constants
#define MAXAIM 0.24f
#define MINAIM 0.06f
#define AIMDIFF 0.16f
#define AIMMOVE 0.01f
#define STEADY 0.006f

// zombie behavior constants
#define MAXSIGHTDIST 9
#define MAXSEARCHDIST 9
#define SEARCHPROB 4
#define ATTACKTIME 30
#define COUNTERTIME 3
#define PUNCHRATIO 3

// constants for zombie search algorithms
#define HEURISTICWEIGHT 1.5f 
#define MAXEXTENDED 64
#define EDGELENGTH 64
#define UNCERTAINTY 8

struct bullet {
    fpoint pos, vel;
    bool active;

    bullet();
};

class zSprite: public Sprite {
    public:
        short state, stateData, dir;
        point square;
        bool isPlayer;

        zSprite();
        virtual void setSquare(const int=0, const int=0, const bool=false);
        void face(zSprite* const);
        virtual bool ready() = 0;
        void setState(const int);
        void animate();
};

class Player: public zSprite {
    public:
        float stats[NUMSTATS], aim;
        short equipped;
        bool runHeld, punchHeld, fireHeld;
        bool running, usingItem;

        Player();
        bool ready();
        void changeStat(const int, const float);
};

struct queuedVertex {
    short label, distance, parentDir;
    point vertex;

    queuedVertex(const float, const int, const int, const point);
};

struct compareQueued {
    bool operator()(const queuedVertex a, const queuedVertex b) const;
};

class Zombie: public zSprite {
    public:
        list<point> plan;
        int health;

        Zombie();
        void setSquare(const int=0, const int=0, const bool=false);
        bool ready();
        bool attack(zSprite* const, const bool);
        void lookForPlayer(bool (*)(const point), const int, const int, const point);
        void walkRandomly(bool (*)(const point), const int);
        void walkToTarget(bool (*)(const point), const int, const point);

    private:
        void simplifyPlan(bool (*)(const point));
};

bool highToLow (const zSprite* const, const zSprite* const);

