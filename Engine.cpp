
#include "Engine.h"
#include "GUI.cpp"
#include "Animations.cpp"

#ifdef EMSCRIPTEN
#include "emscripten.h"
#define SDL_DisplayFormat SDL_DisplayFormatAlpha
#endif

void HandleEvent(SDL_Event event) {
    switch (event.type) {
        case SDL_QUIT:
            gameover = 1;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    gameover = 1;
                    break;
                case SDLK_w:
                    addItemToList(&moveDir, MOVEUP);
                    break;
                case SDLK_s:
                    addItemToList(&moveDir, MOVEDOWN);
                    break;
                case SDLK_a:
                    addItemToList(&moveDir, MOVELEFT);
                    break;
                case SDLK_d:
                    addItemToList(&moveDir, MOVERIGHT);
                    break;
                case SDLK_LSHIFT:
                    if (!player->runHeld) {
                        player->runHeld = true;
                        player->setState(RUN);
                    }
                    break;
                case SDLK_RETURN:
                    if (!player->punchHeld) {
                        player->punchHeld = true;
                        player->setState((player->equipped == KNIFE ? SWING : PUNCH));
                    }
                    break;
                case SDLK_q:
                    player->equipped = (player->equipped == GUN ? NONE : GUN); 
                    player->setState(EQUIP);
                    break;
                case SDLK_e:
                    player->equipped = (player->equipped == KNIFE ? NONE : KNIFE); 
                    player->setState(EQUIP);
                    break;
                case SDLK_SPACE:
                    if (player->equipped) {
                        player->equipped = NONE;
                        player->setState(EQUIP);
                    }
                    break;
                case SDLK_c:
                    for (int i = 0; i < numZombies; i++)
                        if (zombies[i]->state != DEAD)
                            zombies[i]->walkToTarget(checkSquare, SPRITESIZE/ZRUNSPEED, player->square);
                    break;
                case SDLK_r:
                    if (player->state == DEAD) {
                        player->state = GRABBED;
                        player->stateData = ESCAPE;
                        player->animNum = 0;
                        
                        player->stats[STA] = MAXSTATS[STA];
                    }
                    break;
            }
            break;

        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
                case SDLK_w:
                    moveDir.remove(MOVEUP);
                    break;
                case SDLK_s:
                    moveDir.remove(MOVEDOWN);
                    break;
                case SDLK_a:
                    moveDir.remove(MOVELEFT);
                    break;
                case SDLK_d:
                    moveDir.remove(MOVERIGHT);
                    break;
                case SDLK_LSHIFT:
                    player->runHeld = false;
                    break;
                case SDLK_RETURN:
                    player->punchHeld = false;
                    break;
            }
            break;

        case SDL_MOUSEMOTION:
            if (player->equipped == GUN) {
                float moved = sqrt(event.motion.xrel*event.motion.xrel + event.motion.yrel*event.motion.yrel);
                player->aim = min(player->aim + AIMMOVE*moved, MAXAIM);
            }

            mouse.x = event.motion.x;
            mouse.y = max((int)event.motion.y - GUIHEIGHT, 0);
            break;

        case SDL_MOUSEBUTTONDOWN:
            if ((event.button.button == SDL_BUTTON_LEFT) && (!player->fireHeld)) {
                player->fireHeld = true;
                if (player->state == GRABBED) {
                    player->setState(PUNCH);
                } else if (player->equipped == GUN) {
                    player->setState(FIRE);
                }
            } else if ((event.button.button == SDL_BUTTON_RIGHT) && (!player->punchHeld)) {
                player->punchHeld = true;
                player->setState((player->equipped == KNIFE ? SWING : PUNCH));
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                player->fireHeld = false;
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                player->punchHeld = false;
            }
            break;
    }
}

#if !defined(__APPLE__) && !defined(WINDOWS)
int main(const int argc, const char** argv) {
#else
int main(int argc, char** argv) {
#endif
    SDL_Rect bound = {0, GUIHEIGHT, GAMEWIDTH, GAMEHEIGHT};
    SDL_Surface* temp;

    srand(time(NULL));

    // initialize SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_WM_SetCaption("Seamless world engine", "Seamless world engine");

    screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, BITDEPTH, 0);
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREENWIDTH, SCREENHEIGHT, BITDEPTH, 0, 0, 0, 0);
    buffer = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);

    SDL_EnableKeyRepeat(0, 0);
    SDL_ShowCursor(SDL_DISABLE);

    initAudio();

    // load world data from file and load first zone
    ifstream worldData("world.dat", ios::in | ios::binary);
    worldData.read((char*)world, WORLDSIZE);
    zone.set(INITZONEX, INITZONEY);
    loadZone();

    // load and draw the tileset sprite
    tileset = new Sprite();
    tileset->loadSprite(SPRITESIZE, SPRITESIZE, "tileset.bmp", 5, 1, screen, bound);
    camera.set(INITCAMERAX, INITCAMERAY);
    redrawTiles();

    // load player sprite
    player = new Player();
    player->loadSprite(SPRITESIZE, SPRITESIZE, "player.bmp", 41, 1, screen, bound);
    player->x = NUMCOLS*SPRITESIZE/2;
    player->y = NUMROWS*SPRITESIZE/2 - ZERO;
    player->setSquare(camera.x, camera.y);

    allSprites.push_back(player);

    // load zombie sprites
    for (int i = 0; i < numZombies; i++) {
        zombies[i] = new Zombie();
        zombies[i]->loadSprite(SPRITESIZE, SPRITESIZE, "zombie.bmp", 25, 1, screen, bound);
        zombies[i]->x = NUMCOLS*SPRITESIZE/2 + ZOMBIESTART*SPRITESIZE*SHIFT[i % 4].x;
        zombies[i]->y = NUMCOLS*SPRITESIZE/2 - ZERO + ZOMBIESTART*SPRITESIZE*SHIFT[i % 4].y;
        zombies[i]->setSquare(camera.x, camera.y);

        allSprites.push_back(zombies[i]);
    }

    // load and hide item sprite
    item = new Sprite();
    item->loadSprite(ITEMSIZE, ITEMSIZE, "item.bmp", 8, 13, screen, bound);

    initGUI();

    SDL_BlitSurface(buffer, NULL, screen, NULL);
    SDL_UpdateRect(screen, 0, 0, 0, 0);
    #ifdef EMSCRIPTEN
    emscripten_set_main_loop(update, FRAMERATE, 1);
    #else
    gameLoop();
    #endif
    cleanup();
    return 0;
}

void loadZone() {
    point minScene = point(zone.x/NUMCOLS, zone.y/NUMROWS);
    point scene = point(minScene);
    point min = point(zone.x % NUMCOLS, zone.y % NUMROWS);
    point max = point(NUMCOLS, NUMROWS);
    point tilesPos, maxScene;

    maxScene.x = minScene.x + ZONESIZE + (zone.x % NUMCOLS == 0 ? 0 : 1);
    maxScene.y = minScene.y + ZONESIZE + (zone.y % NUMROWS == 0 ? 0 : 1);

    for (; scene.x < maxScene.x; scene.x++) {
        for (scene.y = minScene.y; scene.y < maxScene.y; scene.y++) {
            loadZoneFragment(scene, min, max, tilesPos);

            tilesPos.y += max.y - min.y;
            min.y = 0;
            if (scene.y == minScene.y + ZONESIZE - 1)
                max.y = zone.y % NUMROWS;
        }
        tilesPos.set(tilesPos.x + max.x - min.x, 0);
        min.set(0, zone.y % NUMROWS);
        max.y = NUMROWS;
        if (scene.x == minScene.x + ZONESIZE - 1)
            max.x = zone.x % NUMCOLS;
    }
}

void loadZoneFragment(const point scene, const point min, const point max, const point tilesPos) {
    point pos;

    pos.x = tilesPos.x;
    for (int i = min.x; i < max.x; i++) {
        pos.y = tilesPos.y;
        for (int j = min.y; j < max.y; j++) {
            tiles[pos.x][pos.y] = world[scene.x][scene.y][i][j];
            pos.y++;
        }
        pos.x++;
    }
}

void drawTiles(const point diff) {
    SDL_Rect rSrc, rTarget;
    point tile = point(camera.x/SPRITESIZE, camera.y/SPRITESIZE);
    point maxTile;

    rSrc.x = max(diff.x, (short)0);
    rSrc.y = max(diff.y, (short)0) + GUIHEIGHT;
    rSrc.w = GAMEWIDTH - abs(diff.x);
    rSrc.h = GAMEHEIGHT - abs(diff.y);

    rTarget.x = max(-diff.x, 0);
    rTarget.y = max(-diff.y, 0) + GUIHEIGHT;
    rTarget.w = rSrc.w;
    rTarget.h = rSrc.h;

    SDL_BlitSurface(buffer, &rSrc, buffer, &rTarget);

    if (diff.x != 0) {
        maxTile.x = 1;
        int overlap = camera.x % SPRITESIZE - diff.x;
        if ((overlap < 0) || (overlap >= SPRITESIZE))
            maxTile.x = 2;

        tileset->x = -(camera.x % SPRITESIZE);
        tileset->y = -(camera.y % SPRITESIZE);

        if (diff.x > 0) {
            int offset = NUMCOLS + 1 - maxTile.x;
            tile.x += offset;
            tileset->x += SPRITESIZE*offset;
        }
        maxTile.x += tile.x;
        maxTile.y = tile.y + NUMROWS + (camera.y % SPRITESIZE == 0 ? 0 : 1);

        for (; tile.x < maxTile.x; tile.x++) {
            for (tile.y = camera.y/SPRITESIZE; tile.y < maxTile.y; tile.y++) {
                tileset->frameCol = tiles[tile.x][tile.y];
                tileset->draw(buffer);

                tileset->y += SPRITESIZE;
            }
            tileset->x += SPRITESIZE;
            tileset->y = -(camera.y % SPRITESIZE);
        }
    }

    if (diff.y != 0) {
        tile.set(camera.x/SPRITESIZE, camera.y/SPRITESIZE);

        maxTile.y = 1;
        int overlap = camera.y % SPRITESIZE - diff.y;
        if ((overlap < 0) || (overlap >= SPRITESIZE))
            maxTile.y = 2;

        tileset->x = -(camera.x % SPRITESIZE);
        tileset->y = -(camera.y % SPRITESIZE);

        if (diff.y > 0) {
            int offset = NUMROWS + 1 - maxTile.y;
            tile.y += offset;
            tileset->y += SPRITESIZE*offset;
        }
        maxTile.x = tile.x + NUMCOLS + (camera.x % SPRITESIZE == 0 ? 0 : 1);
        maxTile.y += tile.y;

        for (; tile.y < maxTile.y; tile.y++) {
            for (tile.x = camera.x/SPRITESIZE; tile.x < maxTile.x; tile.x++) {
                tileset->frameCol = tiles[tile.x][tile.y];
                tileset->draw(buffer);

                tileset->x += SPRITESIZE;
            }
            tileset->x = -(camera.x % SPRITESIZE);
            tileset->y += SPRITESIZE;
        }
    }
}

void redrawTiles() {
    point minTile = point(camera.x/SPRITESIZE, camera.y/SPRITESIZE);
    point tilesPos = point(minTile);
    point maxTile;

    tileset->x = -camera.x % SPRITESIZE;
    tileset->y = -camera.y % SPRITESIZE;

    maxTile.x = minTile.x + NUMCOLS + (tileset->x == 0 ? 0 : 1);
    maxTile.y = minTile.y + NUMROWS + (tileset->y == 0 ? 0 : 1);

    for (int i = minTile.x; i < maxTile.x; i++) {
        for (int j = minTile.y; j < maxTile.y; j++) {
            tileset->frameCol = tiles[i][j];
            tileset->draw(buffer);

            tileset->y += SPRITESIZE;
        }
        tileset->x = tileset->x + SPRITESIZE;
        tileset->y = -camera.y % SPRITESIZE;
    }
}

void update() {
  frameNum = (frameNum + 1) % MAXFRAMENUM;

  SDL_Event event;
  for (int i = 0; (i < EVENTSPERFRAME) && SDL_PollEvent(&event); i++)
      HandleEvent(event);

  movePlayer();
  for (int i = 0; i < numZombies; i++)
      moveZombie(zombies[i]);
  for (int i = 0; i < numBullets; i++)
      if (bullets[i].active)
          moveBullet(&bullets[i]);
  checkStats();
  scrollCamera();

  SDL_BlitSurface(buffer, NULL, screen, NULL);

  allSprites.sort(highToLow);
  for (list<zSprite*>::iterator it = allSprites.begin(); it != allSprites.end(); it++) {
      if ((*it == player) && (player->usingItem))
          item->draw(screen);
      (*it)->draw(screen);
  }

  for (int i = 0; i < numBullets; i++) {
      if ((bullets[i].active) && (bullets[i].pos.y >= 0))
          for (int j = 0; j < 4; j++)
              SDL_DrawPixel(screen, bullets[i].pos.x + (j % 2), bullets[i].pos.y + GUIHEIGHT + (j/2 % 2), WHITE);
  }

  drawMouse();

  SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void gameLoop() {
    int curTime, lastTime, lastSecond, delay, i;
    int framesPerSecond = 0;
    timeval t;

    gettimeofday(&t, NULL);
    curTime = t.tv_sec*TICKSPERSEC + t.tv_usec;
    lastTime = curTime;
    lastSecond = curTime;

    while (!gameover) {
        gettimeofday(&t, NULL);
        curTime = t.tv_sec*TICKSPERSEC + t.tv_usec;
        if (curTime > lastTime + FRAMEDELAY) {
            lastTime = curTime;

            if (curTime > lastSecond + TICKSPERSEC) {
                //cout << "FPS = " << 1.0f*framesPerSecond*TICKSPERSEC/(curTime - lastSecond) << endl;
                lastSecond = curTime;
                framesPerSecond = 1;
            } else {
                framesPerSecond++;
            }

            update();

            gettimeofday(&t, NULL);
            delay = lastTime + FRAMEDELAY - t.tv_sec*TICKSPERSEC - t.tv_usec - MINDELAY;
            #if !defined(WINDOWS)
            if (delay > 0)
                usleep(delay);
            #endif
        }
    }
}

void movePlayer() {
    player->usingItem = false;

    if (!player->ready()) {
        player->animate();
        return;
    }

    list<int>::iterator i = moveDir.begin();
    fpoint* move = new fpoint();
    float deltaSTA = DELTASTAT[STA];

    float length = (player->running ? PRUNSPEED : PSPEED);
    if (player->equipped == GUN)
        length = (player->running ? GUNRUNSPEED : GUNSPEED);

    if (i != moveDir.end()) {
        player->dir = *i;
        for (; i != moveDir.end(); i++) {
            move->x += SHIFT[*i].x;
            move->y += SHIFT[*i].y;
        }
    } else
        player->animNum = 0;

    move->setLength(length);
    avoidZombies(player, move);
    moveSprite(player, move, ANIMFRAMES/length);

    if (player->equipped)
        displayItem(player, player->frameCol >= 4, player->equipped == GUN);

    if ((player->running) && (move->length() > ZERO))
        deltaSTA = RUNSTA;
    player->changeStat(STA, deltaSTA);

    delete move;
}

void moveZombie(Zombie* const zombie) {
    if (!zombie->ready()) {
        zombie->animate();
        return;
    }

    float length;
    float dist = fpoint(zombie->x - player->x, zombie->y - player->y).length();
    if (dist < SPRITESIZE + TOLERANCE) {
        if (zombie->attack(player, dist < SPRITESIZE)) {
            zombie->animate();
            return;
        } else if (dist < SPRITESIZE)
            return;
    }

    zombie->lookForPlayer(checkSquare, SPRITESIZE/ZRUNSPEED, zombie->dir, player->square);

    if (zombie->stateData <= 0) {
        zombie->walkRandomly(checkSquare, SPRITESIZE/ZSPEED);
    } else {
        zombie->stateData--;
    }

    fpoint* move = new fpoint();

    if (zombie->state == WALKRANDOMLY) {
        move->x += SHIFT[zombie->dir].x;
        move->y += SHIFT[zombie->dir].y;

        length = ZSPEED;
    } else if ((zombie->state == WALKTOTARGET) || (zombie->state == SEARCH)) {
        move->x = SPRITESIZE*zombie->plan.front().x - camera.x - zombie->x;
        move->y = SPRITESIZE*zombie->plan.front().y - camera.y - zombie->y;

        if ((abs(move->x) < TOLERANCE) && (abs(move->y) < TOLERANCE)) {
            zombie->plan.pop_front();
            if (zombie->plan.empty()) {
                zombie->state = PAUSE;
                zombie->stateData = FRAMERATE;
                delete move;
                return;
            }
            move->x = SPRITESIZE*zombie->plan.front().x - camera.x - zombie->x;
            move->y = SPRITESIZE*zombie->plan.front().y - camera.y - zombie->y;
        }

        if (abs(move->x) > abs(move->y)) {
            zombie->dir = (move->x > 0 ? MOVERIGHT : MOVELEFT);
        } else {
            zombie->dir = (move->y > 0 ? MOVEDOWN : MOVEUP);
        }

        length = (zombie->state == WALKTOTARGET ? ZRUNSPEED : ZSPEED);
    } else
        zombie->animNum = 0;

    move->setLength(length);
    avoidZombies(zombie, move);
    moveSprite(zombie, move, ANIMFRAMES/length);
    delete move;
}

void avoidZombies(const zSprite* const sprite, fpoint* move, const int sensitivity) {
    if (move->length() < ZERO)
        return;

    fpoint diff;
    fpoint net = fpoint(0.0f, 0.0f);
    float diffLength;
    bool backoff = (!sprite->isPlayer) && (rand() % 2);

    for (int i = 0; i < numZombies; i++) {
        if ((zombies[i] == sprite) || (zombies[i]->state == DEAD))
            continue;

        diff.set(sprite->x - zombies[i]->x, sprite->y - zombies[i]->y);
        diffLength = diff.length();
        if (diffLength < SEPARATION) {
            diff.setLength(sensitivity/(max(diffLength, MINFORCEDIST)));
            if (sprite->isPlayer) {
                diff.x = (diff.x*move->x > ZERO ? 0 : diff.x);
                diff.y = (diff.y*move->y > ZERO ? 0 : diff.y);
            }
            net.x += diff.x;
            net.y += diff.y;
        }
    }

    if (sprite->isPlayer)
        net *= PFORCE;

    if (net.x*move->x < ZERO)
        move->x = (abs(move->x) < abs(net.x) ? backoff*DIRECTIONALITY*net.x : move->x + net.x);
    if (net.y*move->y < ZERO)
        move->y = (abs(move->y) < abs(net.y) ? backoff*DIRECTIONALITY*net.y : move->y + net.y);
}

void moveSprite(zSprite* const sprite, fpoint* move, const int animFrames) {
    checkSquares(sprite, move);

    if (move->length() > ZERO) {
        sprite->x += move->x;
        sprite->y += move->y;
        sprite->setSquare(camera.x, camera.y);
    }

    if (animFrames) {
        sprite->frameCol = sprite->dir;
        if (move->length() > ZERO) { 
            if (sprite->animNum % (2*animFrames) < animFrames) {
                sprite->frameCol += 4;
                if (sprite->animNum == 0)
                    playSound(sprite, FOOTSTEP);
            }
            sprite->animNum = (sprite->animNum + 1) % (4*animFrames);
        }
    }
}

void moveBullet(bullet* const b) {
    point square;

    for (int i = 0; i < NUMBULLETSTEPS; i++) {
        b->pos += b->vel;

        for (int i = 0; i < numZombies; i++) {
            if (zombies[i]->state == DEAD)
                continue;

            if ((b->pos.x > zombies[i]->x) && (b->pos.x < zombies[i]->x + SPRITESIZE) && \
                    (b->pos.y > zombies[i]->y) && (b->pos.y < zombies[i]->y + SPRITESIZE)) {
                bulletDamage(zombies[i], b);
                b->active = false;
                return;
            }
        }

        square.set((int)((b->pos.x + camera.x)/SPRITESIZE), (int)((b->pos.y + camera.y)/SPRITESIZE));
        if (!checkSquare(square)) {
            b->active = false;
            return;
        }
    }
}

void checkSquares(const zSprite* const sprite, fpoint* move) {
    point square = point(sprite->square);
    fpoint pos = fpoint(sprite->x + camera.x, sprite->y + camera.y);
    point offset = point(0, 0);
    float overlap;
    bool collided = false;
    float speed = move->length();
    if (speed < ZERO)
        return;

    // check if we cross a square boundary while moving up or down
    if ((move->y < -ZERO) && (fmod(pos.y+TOLERANCE, SPRITESIZE) < -move->y + ZERO)) {
        offset.y = -1;
    } else if ((move->y > ZERO) && (fmod(pos.y, SPRITESIZE) > SPRITESIZE - move->y - ZERO)) {
        offset.y = 1;
    }

    // if we cross a vertical boundary, make sure the adjacent square is open
    if (offset.y != 0) {
        overlap = pos.x - SPRITESIZE*square.x;
        offset.x = (overlap > 0 ? 1 : -1);

        if (!checkSquare(square.x, square.y+offset.y)) {
            collided = true;
        // if there's overlap with the square to the right or left, also check that diagonally adjacent blocks are open
        } else if (offset.x*overlap > TOLERANCE) {
            if (!checkSquare(square + offset)) {
                collided = true;
                if ((offset.x*overlap < PUSHAWAY) && (offset.x*move->x <= 0))
                    move->x = -offset.x*speed;
            }
        }

        // if one of those squared is blocked, adjust the velocity y-coordinate as needed
        if (collided) {
            if (offset.y < 0) {
                move->y = (SPRITESIZE - TOLERANCE + ZERO) - fmod(pos.y, SPRITESIZE);
            } else {
                move->y = SPRITESIZE - ZERO - fmod(pos.y, SPRITESIZE);
            }
        }
    }

    // similar checks for the velocity x-coordinates
    offset.x = 0;
    if ((move->x < -ZERO) && (fmod(pos.x+TOLERANCE, SPRITESIZE) < -move->x + ZERO)) {
        offset.x = -1;
    } else if ((move->x > ZERO) && (fmod(pos.x-TOLERANCE, SPRITESIZE) > SPRITESIZE - move->x - ZERO)) {
        offset.x = 1;
    }

    if (offset.x != 0) {
        overlap = pos.y - SPRITESIZE*square.y;
        if ((offset.y != 0) && (!collided) && (!checkSquare(square + offset))) {
            collided = true;
        } else {
            collided = false;
        }
        offset.y = (overlap > 0 ? 1 : -1);

        if (!checkSquare(square.x+offset.x, square.y)) {
            collided = true;
        } else if ((offset.y > 0) || (-overlap > TOLERANCE)) {
            if (!checkSquare(square + offset)) {
                collided = true;
                if ((offset.x*overlap < PUSHAWAY) && (offset.y*move->y <= 0))
                    move->y = (checkSquare(square.x, square.y+offset.y) ? -offset.y*speed : 0);
            }
        }

        if (collided) {
            if (offset.x < 0) {
                move->x = (SPRITESIZE - TOLERANCE + ZERO) - fmod(pos.x, SPRITESIZE);
            } else {
                move->x = TOLERANCE - ZERO - fmod(pos.x, SPRITESIZE);
            }
        }
    }
}

bool checkSquare(const point square) {
    return checkSquare(square.x, square.y);
}

bool checkSquare(const int x, const int y) {
    if ((x < 0) || (x >= ZONESIZE*NUMCOLS))
        return false;
    if ((y < 0) || (y >= ZONESIZE*NUMROWS))
        return false;
    return tiles[x][y] != 4;
}

bool intersect(const Sprite* const sprite, const Sprite* const other) {
    if ((sprite->x + sprite->width > other->x) &&
            (sprite->x < other->x + other->width))
        if ((sprite->y + sprite->height > other->y) &&
                (sprite->y < other->y + other->height))
            return true;

    return false;
}

bool intersectDir(const zSprite* const sprite, const Sprite* const other, const int tolerance) {
    // check if the leading edge of sprite intersects with other, with large tolerance in the
    // sprite's direction, but with small tolerance in the perpendicular direction 
    const short dir = sprite->dir;
    bool checkHor, checkVert;

    if (dir % 2 == 0) {
        checkHor = (sprite->x + sprite->width - TOLERANCE > other->x) &&
            (sprite->x < other->x + other->width - TOLERANCE);
        if (dir == MOVEUP) {
            checkVert = (sprite->y + ZERO > other->y) && (sprite->y < other->y + other->height + tolerance);
        } else {
            checkVert = (sprite->y + sprite->height + tolerance > other->y) && (sprite->y < other->y + ZERO);
        }
    } else {
        if (dir == MOVERIGHT) {
            checkHor = (sprite->x + sprite->width + tolerance > other->x) && (sprite->x < other->x + ZERO);
        } else {
            checkHor = (sprite->x + ZERO > other->x) && (sprite->x < other->x + other->width + tolerance);
        }
        checkVert = (sprite->y + sprite->height - TOLERANCE > other->y) &&
                (sprite->y < other->y + other->height - TOLERANCE);
    }

    return checkHor && checkVert;
}

void scrollCamera() {
    point diff;

    if (player->x < (GAMEWIDTH - BOXSIZE)/2) {
        diff.x = player->x - (GAMEWIDTH - BOXSIZE)/2;
    } else if (player->x + SPRITESIZE > (GAMEWIDTH + BOXSIZE)/2) {
        diff.x = player->x + SPRITESIZE - (GAMEWIDTH + BOXSIZE)/2;
    }

    if (player->y < (GAMEHEIGHT - BOXSIZE)/2) {
        diff.y = player->y - (GAMEHEIGHT - BOXSIZE)/2;
    } else if (player->y + SPRITESIZE > (GAMEHEIGHT + BOXSIZE)/2) {
        diff.y = player->y + SPRITESIZE - (GAMEHEIGHT + BOXSIZE)/2;
    }

    if ((diff.x != 0) || (diff.y != 0)) {
        camera.x += diff.x;
        camera.y += diff.y;

        player->x -= diff.x;
        player->y -= diff.y;
        if (player->equipped) {
            item->x -= diff.x;
            item->y -= diff.y;
            if (player->equipped == GUN)
                oldPos -= diff;
        }
        for (int i = 0; i < numZombies; i++) {
            zombies[i]->x -= diff.x;
            zombies[i]->y -= diff.y;
        }
        for (int i = 0; i < numBullets; i++)
            if (bullets[i].active)
                bullets[i].pos -= fpoint(diff);

        drawTiles(diff);

        if ((camera.x < 0) || (camera.x > 2*GAMEWIDTH) || (camera.y < 0) || (camera.y > 2*GAMEHEIGHT)) {
            point offset;
            offset.x = (player->x + camera.x)/SPRITESIZE - 3*NUMCOLS/2;
            offset.y = (player->y + camera.y)/SPRITESIZE - 3*NUMROWS/2;
            zone.x += offset.x;
            zone.y += offset.y;
            camera.x -= SPRITESIZE*offset.x;
            camera.y -= SPRITESIZE*offset.y;

            player->setSquare(camera.x, camera.y, true);
            for (int i = 0; i < numZombies; i++)
                zombies[i]->setSquare(camera.x, camera.y, true);

            loadZone();
        }
    }
}

void cleanup() {
    delete tileset;
    delete player;
    delete item;
    for (int i = 0; i < numZombies; i++)
        delete zombies[i];

    cleanupGUI();
    cleanupAudio();

    SDL_FreeSurface(buffer);
    SDL_Quit();
}

// add a element to a list, unless it already appears in it maxTimes
void addItemToList(list<int>* const intList, const int newElt, const int maxTimes) {
    list<int>::iterator i;
    int timesFound = 0;

    for (i = intList->begin(); i != intList->end(); i++)
        if (*i == newElt)
            timesFound++;
    if (timesFound < maxTimes)
        intList->push_back(newElt);
}

bool contains(list<int>* const l, const int x) {
    list<int>::iterator i;

    for (i = l->begin(); i != l->end(); i++)
        if (*i == x)
            return true;
    return false;
}

