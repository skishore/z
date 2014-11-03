
void checkStats() {
    int stat = -2;

    player->changeStat(SLP, DELTASTAT[SLP]); 
    player->changeStat(EAT, DELTASTAT[EAT]); 

    for (int i = 0; i < NUMSTATS; i++)
        if (oldStats[i] != (int)round(player->stats[i]))
            stat = (stat > -2 ? -1 : i);

    if (player->stats[STA] < ZERO) {
        if ((player->state == GRABBED) && (player->stateData < ESCAPE)) {
            player->setState(DEAD);
        } if (player->running) {
            player->running = false;
            drawStatus();
        }
    }

    drawBars(stat);

    if (player->equipped == GUN) {
        if (player->aim < ZERO) {
            player->aim = MAXAIM;
        } else {
            float moved = fpoint(player->x - oldPos.x, player->y - oldPos.y).length();
            if (player->state == GRABBED)
                player->aim = min(player->aim + AIMDIFF, MAXAIM);
            else if (player->state != FIRE)
                player->aim = min(max(player->aim - STEADY + AIMDIFF*moved, MINAIM), MAXAIM);
            else if (inRange(player->animNum - 1, FFIRE, FFINISH))
                player->aim = min(player->aim + AIMDIFF, MAXAIM);
        }
        oldPos.set(player->x, player->y);
    } else {
        player->aim = 0;
    }

    for (int i = 0; i < NUMSTATS; i++)
        oldStats[i] = (int)round(player->stats[i]);
}

void setSpriteState(zSprite* const sprite, const int state) {
    switch (state) {
        case RUN:
            if (sprite->isPlayer) {
                ((Player*)sprite)->running = !((Player*)sprite)->running;
                drawStatus();
            }
            break;
        case PUNCH:
            if (sprite->isPlayer) {
                attack(sprite, PUNCH);
            } else {
                if (sprite->ready()) {
                    sprite->state = PUNCH;
                    sprite->stateData = (rand() % 2 == 0 ? 0 : (rand() % 3 < 2 ? 1 : 2));
                    sprite->animNum = 0;
                }
            }
            break;
        case EQUIP:
            if (sprite->ready()) {
                sprite->state = EQUIP;
                sprite->animNum = 0;
                if (sprite->isPlayer)
                    playSound(sprite, EQUIPSOUND, ((Player*)sprite)->equipped);
            }
            break;
        case SWING:
            attack(sprite, SWING);
            break;
        case FIRE:
            if (sprite->ready()) {
                sprite->state = FIRE;
                sprite->animNum = 0;
            }
            break;
        case GRABBED:
            if ((sprite->state != GRABBED) && (sprite->state != DEAD)) {
                sprite->state = GRABBED;
                sprite->stateData = 2*(rand() % ESCAPEINIT);
                sprite->animNum = 0;
            }
            break;
        case DEAD:
            sprite->state = DEAD;
            sprite->frameCol = (sprite->isPlayer ? 40 : 24);
            break;
        case STUN:
            if (sprite->isPlayer) {
                sprite->state = WALK;
                sprite->animNum = 0;
            } else {
                sprite->state = STUN;
                if (sprite->stateData >= COMBO)
                    sprite->stateData = COMBO - 1;
                sprite->animNum = -(BASESTUN + STUNDIFF*sprite->stateData);
            }
            break;
       case GRAB:
            if (sprite->ready()) {
                sprite->state = GRAB;
                sprite->stateData = 0;
                sprite->animNum = 0;
            }
            break;
    }
}

void attack(zSprite* const sprite, const int type) {
    if (sprite->ready()) {
        sprite->state = type;
        sprite->stateData = 0;
        sprite->animNum = 0;
    } else if (sprite->state == type) {
        if ((sprite->stateData % 2 == 0) && (sprite->stateData < 2*(COMBO - 1)))
            sprite->stateData++;
    } else if (sprite->state == GRABBED) {
        if ((sprite->stateData % 2 == 0) && (sprite->stateData < 80))
            sprite->stateData++;
    }
}

void knockBack(zSprite* const sprite, const int combo, const short dir, const int sound) {
    if ((sprite->state == GRABBED) || (sprite->state == DEAD))
        return;

    if ((sound > 0) && (sprite->state != BACK)) {
        int type = (sound == PUNCHED ? (combo < COMBO - 1 ? rand() % 6 : rand() % 2 + 6) : -1);
        playSound(sprite, sound, type);
    }

    sprite->state = BACK;
    sprite->animNum = -(BASEBACK + BACKDIFF*combo);
    sprite->stateData = combo;
    sprite->dir = dir^2;
}

void bulletDamage(zSprite* const sprite, const bullet* const b) {
    knockBack(sprite, BULLETKNOCKBACK, 0, SHOT);

    if (sprite->state == BACK) {
        sprite->dir = (b->vel.x < 0 ? MOVERIGHT : MOVELEFT);
        if (abs(b->vel.x) < abs(b->vel.y))
            sprite->dir = (b->vel.y < 0 ? MOVEDOWN : MOVEUP);
    }

    point hit = point(sprite->x + SPRITESIZE/2 - b->pos.x, sprite->y + HEADY - b->pos.y);
    int damage = HEADSHOTDAMAGE - DAMAGELOST*NUMBULLETSTEPS*abs(-b->vel.y*hit.x + b->vel.x*hit.y)/BULLETSPEED;
    if (!sprite->isPlayer)
        ((Zombie*)sprite)->health = max(((Zombie*)sprite)->health - damage, 0);
}

void animateSprite(zSprite* const sprite) {
    fpoint* move = NULL;

    switch (sprite->state) {
        case BACK:
            move = back(sprite);
            break;
        case PUNCH:
            move = punch(sprite);
            break;
        case EQUIP:
            move = equip(sprite);
            break;
        case SWING:
            move = swing(sprite);
            break;
        case FIRE:
            move = fire(sprite);
            break;
        case GRABBED:
            move = grabbed(sprite);
            break;
        case STUN:
            move = stun(sprite);
            break;
        case GRAB:
            move = grab(sprite);
            break;
    }

    sprite->animNum++;
    if (move != NULL) {
        moveSprite(sprite, move);
        delete move;
    }
}

fpoint* back(zSprite* const sprite) {
    fpoint* move = NULL;

    move = new fpoint(SHIFT[sprite->dir^2].x, SHIFT[sprite->dir^2].y);
    sprite->frameCol = sprite->dir;
    if (sprite->animNum >= -1)
        sprite->setState(STUN);

    if (sprite->isPlayer) {
        move->setLength(PLAYERBACK*BACKSPEED[sprite->stateData]);
    } else {
        move->setLength(BACKSPEED[sprite->stateData]);
    }
    return move;
}

fpoint* punch(zSprite* const sprite) {
    fpoint* move = NULL;
    short combo = sprite->stateData/2;

    if (sprite->animNum >= THROW[combo] + PULLBACK) {
        if (sprite->isPlayer) {
            if (!nextCombo(sprite, FINISH[combo])) {
                sprite->frameCol = sprite->dir + (inRange(sprite->stateData, 2, 4) ? 16 : 8);
                return NULL;
            }
            combo++;
        } else {
            sprite->state = WALKRANDOMLY;
            sprite->stateData = rand() % (int)(2*SPRITESIZE/ZSPEED);
            return NULL;
        }
    }

    if (inRange(sprite->animNum, WINDUP[combo], RIDE[combo])) {
        if (sprite->animNum == WINDUP[combo])
            playSound(sprite, PUNCHSOUND, combo);

        move = new fpoint(SHIFT[sprite->dir]);
        move->setLength(PUNCHSPEED[combo]);

        if (sprite->isPlayer) {
            for (int i = 0; i < numZombies; i++) {
                if (zombies[i]->state == DEAD)
                    continue;

                if (intersectDir(sprite, zombies[i], PUNCHSPEED[combo]))
                    knockBack(zombies[i], combo, sprite->dir, PUNCHED);
            }
        } else if (intersectDir(sprite, player, PUNCHSPEED[combo])) {
            if (sprite->stateData % 2 == 0) {
                player->changeStat(STA, PUNCHSTA + combo*PUNCHSTADIFF);
                sprite->stateData++;
            }
            knockBack(player, combo, sprite->dir, PUNCHED);
        }
    }

    sprite->frameCol = sprite->dir + (inRange(sprite->stateData, 2, 4) ? 16 : 8) \
        + (inRange(sprite->animNum, WINDUP[combo], THROW[combo]) ? 4 : 0);

    if ((move != NULL) && (sprite->isPlayer))
        avoidZombies(sprite, move, (combo + 1)*SENSITIVITY/2);
    return move;
}

fpoint* equip(zSprite* const sprite) {
    sprite->frameCol = sprite->dir + 8;
    if (sprite->dir == MOVEDOWN)
        sprite->frameCol = 26;

    if (sprite->animNum >= EQUIPTIME)
        sprite->setState(STUN);

    return NULL;
}

fpoint* swing(zSprite* const sprite) {
    fpoint* move = NULL;
    short combo = sprite->stateData/2;

    if (sprite->animNum >= STHROW[combo] + PULLBACK) {
        if (!nextCombo(sprite, SFINISH[combo])) {
            sprite->frameCol = sprite->dir + (inRange(sprite->stateData, 2, 4) ? 8 : 16);
            return NULL;
        }
        combo++;
    }

    if (inRange(sprite->animNum, SWINDUP[combo], SRIDE[combo])) {
        move = new fpoint(SHIFT[sprite->dir]);
        move->setLength(SWINGSPEED[combo]);
    }

    sprite->frameCol = sprite->dir + (inRange(sprite->stateData, 2, 4) ? 8 : 16);
    if (inRange(sprite->animNum, SWINDUP[combo], STHROW[combo])) {
        if (sprite->animNum == (2*SWINDUP[combo] + STHROW[combo])/3)
            playSound(sprite, KNIFESOUND, combo);

        fpoint itemPos = fpoint(SHIFT[sprite->dir]);
        
        sprite->frameCol = sprite->dir + 28;
        item->frameCol = 2*sprite->dir;

        if (combo < 2) {
            short frame = 3*(sprite->animNum - SWINDUP[combo])/(STHROW[combo] - SWINDUP[combo]);
            short offset = (combo == 1 ? -frame + 1 : frame - 1);

            sprite->frameCol += 4*offset;
            item->frameCol = (2*sprite->dir + offset + 8) % 8;
            itemPos += fpoint(SHIFT[(sprite->dir + 4 + offset) % 4]);
        }


        itemPos.setLength((SPRITESIZE - ITEMSIZE)/2 + BLADELENGTH + HANDLE);
        item->x = sprite->x + (SPRITESIZE - ITEMSIZE)/2 + itemPos.x;
        item->y = sprite->y + (SPRITESIZE - ITEMSIZE)/2 + itemPos.y;
        if (sprite->dir % 2 == 1)
            item->y += ITEMYOFFSET;
        if (sprite->isPlayer) {
            item->frameRow = (combo == 1 ? 1 : 0) + 2*KNIFETYPE;
            ((Player*)sprite)->usingItem = true;
        }

        if ((combo < COMBO - 1) || (sprite->animNum < SRIDE[combo]))
            for (int i = 0; i < numZombies; i++) {
                if (zombies[i]->state == DEAD)
                    continue;

                if (((combo == 2) && (intersectDir(sprite, zombies[i], BLADELENGTH + HANDLE))) || \
                        (combo < 2) && (intersect(item, zombies[i]))) {
                    if (zombies[i]->state != BACK)
                        zombies[i]->health = max(zombies[i]->health - DAMAGE[combo], 0);
                    knockBack(zombies[i], 0, sprite->dir, KNIFED);
                }
            }
    }

    if ((move != NULL) && (sprite->isPlayer))
        avoidZombies(sprite, move, (combo + 1)*SENSITIVITY/2);
    return move;
}

bool nextCombo(zSprite* const sprite, const short finish) {
    if (sprite->stateData % 2 == 1) {
        sprite->stateData++;
        sprite->animNum = 0;
        return true;
    }
    
    if ((sprite->animNum >= finish - 1) || (moveDir.size() > 0))
        sprite->setState(STUN);
    return false;
}

fpoint* fire(zSprite* const sprite) {
    fpoint* move = NULL;

    displayItem(sprite, false, true);
    point shift = SHIFT[sprite->dir];

    if (inRange(sprite->animNum, FPULL, FFIRE)) {
        move = new fpoint(-KICKBACK*shift.x, -KICKBACK*shift.y);
        item->frameCol++;

        if (sprite->animNum == FPULL) {
            for (int i = 0; i < numBullets; i++)
                if (!bullets[i].active) {
                    bullets[i].pos.set(item->x + ITEMSIZE/2, item->y + ITEMSIZE/2);

                    bullets[i].vel = fpoint(mouse) - bullets[i].pos;
                    bullets[i].vel.setLength(BULLETSPEED/NUMBULLETSTEPS);
                    if (sprite->isPlayer)
                        bullets[i].vel.addGaussianNoise(SIGMA*((Player*)sprite)->aim);

                    bullets[i].active = true;
                    playSound(sprite, GUNSOUND);
                    break;
                }
        }
    } else if (sprite->animNum >= FFINISH) {
        sprite->setState(STUN);
    }

    return move;
}

fpoint* grabbed(zSprite* const sprite) {
    if (sprite->stateData >= ESCAPE) {
        if (sprite->stateData == ESCAPE) {
            sprite->animNum = 0;
            sprite->stateData++;
        }

        sprite->frameCol = (sprite->dir + (sprite->animNum/ANIMFRAMES)) % 4 + 12;
        if (inRange(sprite->animNum, 4*ANIMFRAMES, 6*ANIMFRAMES)) {
            sprite->frameCol = sprite->dir + 12;

            for (int i = 0; i < numZombies; i++) {
                if (zombies[i]->state == DEAD)
                    continue;

                if (intersect(sprite, zombies[i])) {
                    knockBack(zombies[i], COMBO - (rand() % 3), zombies[i]->dir^2, PUNCHED);
                } else if ((zombies[i]->state == GRAB) && (zombies[i]->stateData > 0)) {
                    zombies[i]->state = WALKRANDOMLY;
                    zombies[i]->stateData = rand() % (int)(2*SPRITESIZE/ZSPEED);
                }
            }
        } else if (sprite->animNum >= 6*ANIMFRAMES - 1) {
            sprite->setState(STUN);
        }

        return NULL;
    }

    if (sprite->animNum == 0)
        sprite->frameCol = sprite->dir;

    if (sprite->stateData % 2 == 1) {
        sprite->frameCol = sprite->dir + 4*(rand() % 3);
        sprite->stateData++;
    }

    if (!sprite->isPlayer)
        return NULL;
    fpoint* move = NULL;

    for (int i = 0; i < numZombies; i++) {
        if (zombies[i]->state == GRAB) {
            float damage = (zombies[i]->stateData < LONGGRAB ? GRABSTA : LONGGRABSTA);
            ((Player*)sprite)->changeStat(STA, damage);

            if (zombies[i]->animNum % (PULLA + PULLB) == 0) {
                if (move == NULL)
                    move = new fpoint(0.0f, 0.0f);
                move->x += zombies[i]->x - sprite->x;
                move->y += zombies[i]->y - sprite->y;
            }
        }
    }

    if (move != NULL)
        move->setLength(PULLSPEED);
    return move;
}

fpoint* stun(zSprite* const sprite) {
    if (sprite->isPlayer)
        return NULL;

    if (((Zombie*)sprite)->health == 0)
        sprite->setState(DEAD);

    if (sprite->animNum >= -1) {
        sprite->state = WALKRANDOMLY;
        sprite->stateData = SPRITESIZE/ZSPEED + rand() % (int)(2*SPRITESIZE/ZSPEED);
    }

    return NULL;
}

fpoint* grab(zSprite* const sprite) {
    if (sprite->stateData > 0) {
        sprite->frameCol = sprite->dir + (sprite->animNum % (PULLA + PULLB) < PULLB ? 0 : 20);
        if (sprite->stateData < LONGGRAB)
            sprite->stateData++;
        return NULL;
    }

    fpoint* move = NULL;

    sprite->frameCol = sprite->dir + 16;
    if (inRange(sprite->animNum, GWINDUP, GTHROW)) {
        if (sprite->animNum == GWINDUP)
            playSound(sprite, PUNCHSOUND);

        sprite->frameCol = sprite->dir + 20;

        move = new fpoint(player->x - sprite->x, player->y - sprite->y);
        move->setLength(PRUNSPEED + ZRUNSPEED - PSPEED);

        if (intersectDir(sprite, player, -PRUNSPEED)) {
            player->setState(GRABBED);
            sprite->stateData = 1;
            sprite->animNum = PULLA;
        }
    } else if (sprite->animNum >= GFINISH - 1) {
        sprite->state = WALKRANDOMLY;
        sprite->stateData = rand() % (int)(2*SPRITESIZE/ZSPEED);
        sprite->face(player);
    }

    return move;
}

void displayItem(zSprite* const sprite, const bool animated, const bool gun) {
    int length = BLADELENGTH;

    if (gun) {
        sprite->dir = ((sprite->x + SPRITESIZE/2) < mouse.x ? MOVERIGHT : MOVELEFT);
        if (abs(sprite->x + SPRITESIZE/2 - mouse.x) < abs(sprite->y + SPRITESIZE/2 - mouse.y))
            sprite->dir = ((sprite->y + SPRITESIZE/2) < mouse.y ? MOVEDOWN : MOVEUP);

        sprite->frameCol = sprite->dir + (animated ? 12 : 36);
        length = GUNLENGTH;
    }

    point itemOffset = SHIFT[sprite->dir]*(length + HANDLE);
    short frame = 2*sprite->dir + animated + 8*gun;
    

    item->x = sprite->x + ITEMOFFSET[frame].x + itemOffset.x;
    item->y = sprite->y + ITEMOFFSET[frame].y + itemOffset.y;
    item->frameCol = 2*sprite->dir;
    item->frameRow = (gun ? 12 : (sprite->dir == MOVERIGHT ? 0 : 1) + 2*KNIFETYPE);
        
    if (sprite->isPlayer)
        ((Player*)sprite)->usingItem = true;
}   

