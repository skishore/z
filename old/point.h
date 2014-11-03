
#include <complex>
#include <list>

// zero cutoff for float calculations, and other numerical constants
#define ZERO 0.001f
#define RANDMAX 256
#define TWOPI 6.283f

// movement direction constants
#define MOVEUP 0
#define MOVERIGHT 1
#define MOVEDOWN 2
#define MOVELEFT 3

int sign(const int x) {
    return (x > 0) - (x < 0);
}

bool inRange(const int x, const int a, const int b) {
    return (a <= x) && (x < b);
}

struct point {
    short x, y;

    point(int x0=0, int y0=0) {
        x = x0;
        y = y0;
    }

    point(float x0, float y0) {
        x = (short)x0;
        y = (short)y0;
    }

    bool operator==(const point &other) const {
        return ((x == other.x) && (y == other.y));
    }

    point operator+(const point &other) const {
        return point(x + other.x, y + other.y);
    }

    point operator-(const point &other) const {
        return point(x - other.x, y - other.y);
    }

    point operator*(const int &scale) const {
        return point(scale*x, scale*y);
    }

    void set(int newX, int newY) {
        x = newX;
        y = newY;
    }

    float length() {
        return sqrt(x*x + y*y);
    }
};

struct comparePoints {
    bool operator()(const point a, const point b) const {
        return ((a.x < b.x) || ((a.x == b.x) && (a.y < b.y)));
    };
};

const point SHIFT[4] = \
        {point(0, -1), point(1, 0), point(0, 1), point(-1, 0)};

bool lineOfSight(bool (*checkSquare)(const point), const point source, const point target, const point center=point(0, 0)) {
    point cur = source;
    point offset = point((source.x < target.x ? 1 : -1), (source.y < target.y ? 1 : -1));
    int rise, run, nextHeight;

    while (cur.x != target.x) {
        rise = (target.y - source.y)*(2*(cur.x - source.x) + offset.x - center.x) + (1 + center.y)*(target.x - source.x);
        run = 2*(target.x - source.x);

        nextHeight = source.y + rise/run + offset.y;
        if ((rise*run < 0) && (rise % run != 0))
            nextHeight -= 1;
        for (; cur.y != nextHeight; cur.y += offset.y) {
            if (!checkSquare(cur))
                return false;
        }

        if (rise % run == 0) {
            if ((offset.y < 0) && (!checkSquare(cur))) {
                return false;
            } else if ((offset.y > 0) && (!checkSquare(point(cur.x + offset.x, cur.y - 2)))) {
                return false;
            }
        }

        cur.set(cur.x + offset.x, nextHeight - offset.y);
    }

    for (; cur.y != target.y; cur.y += offset.y) {
        if (!checkSquare(cur))
            return  false;
    }

    return true;
}

bool lineOfMovement(bool (*checkSquare)(const point), const point source, const point target) {
    if (abs(target.x - source.x) > abs(target.y - source.y)) {
        return (lineOfSight(checkSquare, source, target, SHIFT[0]) && \
                lineOfSight(checkSquare, source, target, SHIFT[2]));
    } else {
        return (lineOfSight(checkSquare, source, target, SHIFT[3]) && \
                lineOfSight(checkSquare, source, target, SHIFT[1]));
    }
}

list<point>* lineOfSightPath(const point source, const point target) {
    list<point>* path = new list<point>();
    point cur = source;
    point offset = point((source.x < target.x ? 1 : -1), (source.y < target.y ? 1 : -1));
    int rise, run, nextHeight;

    while (cur.x != target.x) {
        rise = (target.y - source.y)*(2*(cur.x - source.x) + offset.x) + (target.x - source.x);
        run = 2*(target.x - source.x);

        nextHeight = source.y + rise/run + offset.y;
        if ((rise*run < 0) && (rise % run != 0))
            nextHeight -= 1;
        for (; cur.y != nextHeight; cur.y += offset.y)
            path->push_back(cur);

        cur.set(cur.x + offset.x, nextHeight - offset.y);
    }

    if (!path->empty())
        path->pop_front();
    for (; cur.y != target.y + offset.y; cur.y += offset.y)
        path->push_back(cur);

    return path;
}

struct fpoint {
    float x, y;

    fpoint(float x0=0.0f, float y0=0.0f) {
        x = x0;
        y = y0;
    }

    fpoint(point p) {
        x = p.x;
        y = p.y;
    }

    bool operator==(const fpoint &other) const {
        return ((abs(x - other.x) < ZERO) && (abs(y - other.y) < ZERO));
    }

    fpoint operator+(const fpoint &other) const {
        return fpoint(x + other.x, y + other.y);
    }

    fpoint operator-(const fpoint &other) const {
        return fpoint(x - other.x, y - other.y);
    }

    void operator+=(const fpoint &other) {
        x += other.x;
        y += other.y;
    }

    void operator-=(const fpoint &other) {
        x -= other.x;
        y -= other.y;
    }

    void operator*=(const float scale) {
        x *= scale;
        y *= scale;
    }

    void set(float newX, float newY) {
        x = newX;
        y = newY;
    }

    float length() {
        return sqrt(x*x + y*y);
    }

    void setLength(float newLength) {
        float length = this->length();
        if (length < ZERO)
            return;

        x = x*newLength/length;
        y = y*newLength/length;
    }

    void normalize() {
        setLength(1);
    }

    void addGaussianNoise(float sigma) {
        float u1 = float((rand() % (RANDMAX - 1)) + 1)/float(RANDMAX);
        float u2 = float((rand() % (RANDMAX - 1)) + 1)/float(RANDMAX);

        float r = sigma*sqrt(-2*log(u1));
        x += r*sin(TWOPI*u1);
        y += r*cos(TWOPI*u1);
    }
};

