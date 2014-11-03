
// general sprite states
#define WALK 0
#define RUN 1
#define BACK 2
#define PUNCH 3
#define EQUIP 4
#define SWING 5
#define FIRE 6
#define GRABBED 7
#define DEAD 8

// zombie states
#define STUN 9
#define PAUSE 10
#define WALKRANDOMLY 11
#define WALKTOTARGET 12 
#define SEARCH 13
#define GRAB 14

// all melee attacks have a three-hit combo 
#define COMBO 3

// timing constants for sprite knockback animation
#define BASEBACK 4
#define BACKDIFF 1
#define BASESTUN 12
#define STUNDIFF 8
const float BACKSPEED[COMBO] = {2.80f, 3.10f, 4.30f};
#define PLAYERBACK 0.80f

// timing constants for the jab and punch
const short WINDUP[COMBO] = {6, 9, 18};
const short RIDE[COMBO] = {10, 15, 24};
const short THROW[COMBO] = {18, 24, 36};
#define PULLBACK 2
const short FINISH[COMBO] = {27, 33, 38};
const float PUNCHSPEED[COMBO] = {0.80f, 1.25f, 2.50f};

// weapon timing and size constants
#define EQUIPTIME 4
#define KNIFETYPE 1
#define BLADELENGTH (KNIFETYPE + 3)
#define GUNLENGTH 5

// timing constants for the melee weapon swing
const short SWINDUP[COMBO] = {6, 12, 16};
const short SRIDE[COMBO] = {9, 16, 22};
const short STHROW[COMBO] = {15, 24, 30};
const short SFINISH[COMBO] = {30, 36, 40};
const float SWINGSPEED[COMBO] = {0.80f, 1.25f, 1.90f};
#define HANDLE 1
#define ITEMYOFFSET 2
const int DAMAGE[COMBO] = {1, 1, 4};

// timing constants for firing the pistol
const short FPULL = 8;
const short FFIRE = 10;
const short FFINISH = 12;
#define KICKBACK 0.40f

// constants for calculating bullet knockback and damage
#define BULLETKNOCKBACK 1
#define HEADY 5
#define HEADSHOTDAMAGE 19
#define DAMAGELOST 1.4f

// timing constants for the zombie's grab and pull
#define GWINDUP 6
#define GTHROW 20
#define GFINISH 28
#define PULLA 56
#define PULLB 20
#define PULLSPEED 1.7f
#define LONGGRAB (PULLA + PULLB)
#define ESCAPE 14
#define ESCAPEINIT 4

// position of the player's equipped item: the first 8 frames are
// for melee weapons, and the next 8 are for ranged 
const point ITEMOFFSET[16] = \
        {point(5, 7), point(6, 6), point(0, 6), point(1, 6), \
        point(-4, 2), point(-3, 3), point(4, 6), point(3, 6), \
        point(4, 1), point(4, 0), point(2, 4), point(3, 4), \
        point(1, 4), point(0, 4), point(2, 4), point(1, 4)};

int oldStats[NUMSTATS];
fpoint oldPos;

void checkStats();

void setSpriteState(zSprite* const, const int);
void attack(zSprite* const, const int);
void knockBack(zSprite* const, const int, const short, const int=0);
void bulletDamage(zSprite* const, const bullet* const);
void animateSprite(zSprite* const);

fpoint* back(zSprite* const);
fpoint* punch(zSprite* const);
fpoint* equip(zSprite* const);
fpoint* swing(zSprite* const);
bool nextCombo(zSprite* const, const short);
fpoint* fire(zSprite* const);
fpoint* grabbed(zSprite* const);
fpoint* stun(zSprite* const);
fpoint* grab(zSprite* const);

void displayItem(zSprite* const, const bool, const bool);

