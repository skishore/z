
//#include "SDL_mixer/SDL_mixer.h"
#include <iostream>

//const int audio_rate = MIX_DEFAULT_FREQUENCY;
//const Uint16 audio_format = AUDIO_S16SYS;
//const int audio_channels = 2;
//const int audio_buffers = 256;
#define NUMCHANNELS 12

#define BASEDIST 1.05f
#define MAXDIST 254
#define BASEPAN 0.8f
#define MAXPAN 254

#define SOUNDTYPES 8
#define FOOTSTEP 0
#define PUNCHSOUND 1
#define PUNCHED 2
#define EQUIPSOUND 3
#define KNIFESOUND 4
#define KNIFED 5
#define GUNSOUND 6
#define SHOT 7
const int NUMSOUNDS[SOUNDTYPES] = {8, 3, 8, 3, 3, 5, 4, 8};
const char* SOUNDNAMES[SOUNDTYPES] = \
        {"step", "punch", "punched", "equip", "swing", "knifed", "fire", "shot"};
//#define VOL MIX_MAX_VOLUME
//const int VOLUME[SOUNDTYPES] = {3*VOL/8, VOL/3, VOL/3, VOL, 2*VOL/3, VOL/6, VOL, VOL};
//Mix_Chunk*** chunks;

void initAudio() {};
void playSound(const zSprite* const, const int, const int=-1) {};
void cleanupAudio() {};

void assert(const bool, const char*) {};

