
#include "zSprite.h"
#include "Animations.h"

bullet::bullet() {
    active = false;
};

zSprite::zSprite(): Sprite() { 
    dir = 0;
    square.set(0, 0);
    isPlayer = false;
};

void zSprite::setSquare(const int cameraX, const int cameraY, const bool newZone) {
    square.x = round((x + cameraX)/width);
    square.y = round((y + cameraY)/height);
};

void zSprite::face(zSprite* const other) {
    if (abs(other->x - x) > abs(other->y - y)) {
        dir = (other->x > x ? MOVERIGHT : MOVELEFT);
    } else {
        dir = (other->y > y ? MOVEDOWN : MOVEUP);
    }
};

void zSprite::setState(const int state) {
    setSpriteState(this, state);
};

void zSprite::animate() {
    animateSprite(this);
};

Player::Player(): zSprite() {
    state = WALK;
    isPlayer = true;

    for (int i = 0; i < NUMSTATS; i++)
        stats[i] = MAXSTATS[i];
    aim = 0;
    equipped = NONE;

    runHeld = false;
    punchHeld = false;
    fireHeld = false;
    running = false;
    usingItem = false;
};

bool Player::ready() {
    return state == WALK;
}

void Player::changeStat(const int stat, const float delta) {
    stats[stat] = min(max(stats[stat] + delta, 0.0f), (float)MAXSTATS[stat]);
    if (stat != STA) {
        stats[STA] = min(stats[STA], stats[stat]);
    } else if (delta > 0) {
        for (int i = 0; i < STA; i++)
            stats[STA] = min(stats[STA], stats[i]);
    }
}

queuedVertex::queuedVertex(const float l, const int dist, const int dir, const point vert) {
    label = (short)l;
    distance = dist;
    parentDir = dir;
    vertex = vert;
};

bool compareQueued::operator()(const queuedVertex a, const queuedVertex b) const {
    return (a.label < b.label) || \
            ((a.label == b.label) && (a.vertex.x < b.vertex.x)) || \
            ((a.label == b.label) && (a.vertex.x == b.vertex.x) && (a.vertex.y < b.vertex.y));
};

Zombie::Zombie(): zSprite() {
    state = PAUSE;
    stateData = 0;

    health = MAXHEALTH;
};

void Zombie::setSquare(const int cameraX, const int cameraY, const bool newZone) {
    point oldSquare = square;

    zSprite::setSquare(cameraX, cameraY);
    if (!newZone)
        return;

    oldSquare = square - oldSquare;
    for (list<point>::iterator it = plan.begin(); it != plan.end(); it++)
        *it = *it + oldSquare;
}

bool Zombie::ready() {
    return (state == PAUSE) || (state == WALKRANDOMLY) || \
            (state == WALKTOTARGET) || (state == SEARCH);
}

bool Zombie::attack(zSprite* const sprite, const bool close) {
    bool attack = rand() % ATTACKTIME == 0;
    bool counter;

    face(sprite);
    counter = (sprite->state == SWING) && (sprite->dir != dir) && (rand() % COUNTERTIME == 0);
    
    if (attack || counter) {
        int attackType = (!close || (rand() % PUNCHRATIO == 0) ? GRAB : PUNCH);
        setState(attackType);
        return true;
    }

    frameCol = dir;
    return false;
};

void Zombie::lookForPlayer(bool (*checkSquare)(const point), const int baseTime,
        const int faceDir, const point playerPos) {
    if ((state == WALKTOTARGET) && (playerPos == plan.back()))
        return;

    point diff = playerPos - square;
    bool facing = (SHIFT[faceDir].x*sign(diff.x) + SHIFT[faceDir].y*sign(diff.y) >= 0);

    if ((diff.length() < MAXSIGHTDIST) && (facing) && (lineOfSight(checkSquare, square, playerPos))) {
        plan = *lineOfSightPath(square, playerPos);
        state = WALKTOTARGET;
        stateData = (baseTime + 1) * diff.length();
        simplifyPlan(checkSquare);
    } else if (state == WALKTOTARGET) {
        int dist = abs(playerPos.x - plan.back().x) + abs(playerPos.y - plan.back().y);

        if ((dist == 1) && (rand() % SEARCHPROB < SEARCHPROB - 1))
            plan.push_back(playerPos);
    }
};

void Zombie::walkRandomly(bool (*checkSquare)(const point), const int baseTime) {
    if ((state == WALKTOTARGET) && (plan.size() > 0)) {
        point diff = plan.back() - square;
        int dist = abs(diff.x) + abs(diff.y);

        if ((dist > 0) && (dist < MAXSEARCHDIST)) {
            state = SEARCH;
            stateData = (baseTime + 1)*dist;
            return;
        }
    }

    if (state == PAUSE) {
        int tries = 0;
        short newDir = rand() % 4;

        while ((tries < 4) && (!checkSquare(square + SHIFT[newDir]))) {
            newDir = rand() % 4;
            tries += 1;
        }

        state = WALKRANDOMLY;
        dir = newDir;
        stateData = baseTime + rand() % (4*baseTime);
    } else {
        state = PAUSE;
        stateData = rand() % (2*baseTime) - baseTime;
    }
};

void Zombie::walkToTarget(bool (*checkSquare)(const point), const int baseTime, const point target) {
    map<point, int, comparePoints> parent;
    priority_queue<queuedVertex, vector<queuedVertex>, compareQueued> agenda;
    point cur, next, diff;
    short dist, nextDist, dir;

    // track the closest vertex found so far and abort after extending too many vertices
    short numExtended = 0;
    point best;
    float bestDist = 4096.0f;

    agenda.push(queuedVertex(bestDist, 0, -1, square));
    while ((!agenda.empty()) && (numExtended < MAXEXTENDED)) {
        if (parent.count(agenda.top().vertex)) {
            agenda.pop();
            continue;
        }

        cur = agenda.top().vertex;
        dir = agenda.top().parentDir;
        parent[cur] = dir;

        diff.set(cur.x - target.x, cur.y - target.y);
        if (diff.length() < bestDist) {
            bestDist = diff.length();
            best = cur;
            if (best == target)
                break;
        }

        dist = agenda.top().distance + EDGELENGTH;
        agenda.pop();

        for (int i = 0; i < 4; i++) {
            if ((i + 2) % 4 == dir)
                continue;
            next.set(cur.x + SHIFT[i].x, cur.y + SHIFT[i].y);
            if ((!parent.count(next)) && (checkSquare(next))) {
                diff.set(next.x - target.x, next.y - target.y);
                nextDist = dist + rand() % UNCERTAINTY;
                agenda.push(queuedVertex(-nextDist - HEURISTICWEIGHT*EDGELENGTH*diff.length(), \
                        nextDist, i, next));
            }
        }
        numExtended++;
    }

    plan.clear();
    if (parent[best] >= 0) {
        while (parent[best] >= 0) {
            plan.push_front(best);
            best = best - SHIFT[parent[best]];
        }

        state = WALKTOTARGET;
        stateData = (baseTime + 1)*plan.size();
    } else {
        state = PAUSE;
        stateData = 0;
    }

    simplifyPlan(checkSquare);
};

void Zombie::simplifyPlan(bool (*checkSquare)(const point)) {
    vector<point> longPlan = vector<point>();
    int cur, low, mid, high;

    longPlan.push_back(square);
    while (!plan.empty()) {
        longPlan.push_back(plan.front());
        plan.pop_front();
    }

    cur = 0;
    while (cur < longPlan.size() - 2) {
        low = cur + 1;
        high = low + 1;

        while (true) {
            high = min(2*high - low, (int)(longPlan.size() - 1));
            if (!lineOfMovement(checkSquare, longPlan[cur], longPlan[high])) {
                break;
            } else if (high == longPlan.size() - 1) {
                plan.push_back(longPlan[high]);
                return;
            }
        }

        while (high > low + 1) {
            mid = (low + high)/2;
            if (lineOfMovement(checkSquare, longPlan[cur], longPlan[mid])) {
                low = mid;
            } else {
                high = mid;
            }
        }

        cur = low;
        plan.push_back(longPlan[cur]);
    }

    for (cur++; cur < longPlan.size(); cur++)
        plan.push_back(longPlan[cur]);
};

bool highToLow (const zSprite* const sprite, const zSprite* const other) {
    if (sprite->state == DEAD)
        return !((other->state == DEAD) && (sprite->y > other->y));
    return (sprite->y < other->y) && (other->state != DEAD);
};

