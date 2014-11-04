#ifndef __SKISHORE_POINT_H__
#define __SKISHORE_POINT_H__

#include <complex>
#include <iostream>

namespace skishore {

struct Point {
  int x, y;

  Point(int x0=0, int y0=0) : x(x0), y(y0) {};

  Point(double x0, double y0) : x((int)x0), y((int)y0) {};

  bool operator==(const Point& other) const {
    return ((x == other.x) && (y == other.y));
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

  float length() const {
    return sqrt(x*x + y*y);
  }
};

std::ostream& operator<<(std::ostream& out, const Point& point) {
  return out << "Point(" << point.x << ", " << point.y << ")";
}

} // namespace skishore

#endif  // __SKISHORE_POINT_H__
