#ifndef __SKISHORE_POINT_H__
#define __SKISHORE_POINT_H__

#include <complex>
#include <iostream>

namespace skishore {

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

} // namespace skishore

#endif  // __SKISHORE_POINT_H__
