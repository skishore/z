
#include "Audio.h"

void initAudio() {
    char name[32];

    if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers))
        cout << "Unable to open audio" << endl;
    Mix_AllocateChannels(NUMCHANNELS);

    chunks = new Mix_Chunk**[SOUNDTYPES];
    for (int i = 0; i < SOUNDTYPES; i++) {
        chunks[i] = new Mix_Chunk*[NUMSOUNDS[i]];

        for (int j = 0; j < NUMSOUNDS[i]; j++) {
            sprintf(name, "Sounds/%s%d.wav", SOUNDNAMES[i], j);
            chunks[i][j] = Mix_LoadWAV(name);
            Mix_VolumeChunk(chunks[i][j], VOLUME[i]);
        }
    }
}

void playSound(const zSprite* const sprite, const int type, const int flags) {
    fpoint offset = fpoint(sprite->x - GAMEWIDTH/2, sprite->y - GAMEHEIGHT/2);
    int dist = BASEDIST*offset.length();
    if (dist > MAXDIST)
        return;

    int right = min(max((int)(BASEPAN*offset.x + MAXPAN/2), 0), MAXPAN);
    int chunk = (flags < 0 ? rand() % NUMSOUNDS[type] : flags);

    for (int i = 0; i < NUMCHANNELS; i++) {
        if (!Mix_Playing(i)) {
            Mix_SetDistance(i, dist);
            Mix_SetPanning(i, MAXPAN - right, right);
            Mix_PlayChannel(i, chunks[type][chunk], 0);

            return;
        }
    }
}

void cleanupAudio() {
    for (int i = 0; i < SOUNDTYPES; i++) {
        for (int j = 0; j < NUMSOUNDS[i]; j++)
            Mix_FreeChunk(chunks[i][j]);
        delete[] chunks[i];
    }
    delete[] chunks;

    Mix_CloseAudio();
}

void assert(const bool assertion, const char* errMessage) {
    if (!assertion)
        cout << errMessage << endl;
}

