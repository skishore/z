
#include <complex>

// zero cutoff for float calculations
#define ZERO 0.01f

struct vector {
    float x, y;

    vector() {
        x = 0;
        y = 0;
    }

    vector(float initX, float initY) {
        x = initX;
        y = initY;
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
};

