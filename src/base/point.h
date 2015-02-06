#ifndef __BABEL_BASE_POINT_H__
#define __BABEL_BASE_POINT_H__

#include <complex>
#include <iostream>

#include "debug.h"

namespace babel {

struct Point {
  int x, y;

  Point(int x0=0, int y0=0) : x(x0), y(y0) {};

  bool operator==(const Point& other) const {
    return ((x == other.x) && (y == other.y));
  }

  bool operator!=(const Point& other) const {
    return ((x != other.x) || (y != other.y));
  }

  void operator+=(const Point& other) {
    x += other.x;
    y += other.y;
  }

  void operator-=(const Point& other) {
    x -= other.x;
    y -= other.y;
  }

  void operator*=(const int& scale) {
    x *= scale;
    y *= scale;
  }

  Point operator+(const Point& other) const {
    return Point(x + other.x, y + other.y);
  }

  Point operator-(const Point& other) const {
    return Point(x - other.x, y - other.y);
  }

  Point operator*(const int& scale) const {
    return Point(scale*x, scale*y);
  }

  Point operator/(const int& scale) const {
    return Point(x/scale, y/scale);
  }

  bool zero() const {
    return x == 0 && y == 0;
  }

  double length() const {
    long long x_ = x;
    long long y_ = y;
    return sqrt(x_*x_ + y_*y_);
  }

  void set_length(double new_length) {
    if (zero()) {
      return;
    }
    double scale = new_length/length();
    x *= scale;
    y *= scale;
  }
};

inline Point operator*(const int& scale, const Point& point) {
  return point*scale;
}

inline std::ostream& operator<<(std::ostream& out, const Point& point) {
  return out << "Point(" << point.x << ", " << point.y << ")";
}

} // namespace babel

namespace std {

template<> struct hash<babel::Point> {
  size_t operator()(const babel::Point& point) const {
    std::hash<int> hasher;
    size_t x_hash = hasher(point.x);
    return hasher(point.y) + 0x9e3779b9 + (x_hash << 6) + (x_hash >> 2);
  }
};

}  // namespace std

#endif  // __BABEL_BASE_POINT_H__
