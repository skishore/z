#ifndef __SKISHORE_POINT_H__
#define __SKISHORE_POINT_H__

#include <complex>
#include <iostream>

namespace skishore {

template <typename T>
struct TPoint {
  T x, y;

  TPoint<T>(T x0=0, T y0=0) : x(x0), y(y0) {};

  bool operator==(const TPoint<T>& other) const {
    return ((x == other.x) && (y == other.y));
  }

  bool operator!=(const TPoint<T>& other) const {
    return ((x != other.x) || (y != other.y));
  }

  void operator+=(const TPoint<T>& other) {
    x += other.x;
    y += other.y;
  }

  void operator-=(const TPoint<T>& other) {
    x -= other.x;
    y -= other.y;
  }

  TPoint<T> operator+(const TPoint<T>& other) const {
    return TPoint<T>(x + other.x, y + other.y);
  }

  TPoint<T> operator-(const TPoint<T>& other) const {
    return TPoint<T>(x - other.x, y - other.y);
  }

  TPoint<T> operator*(const T& scale) const {
    return TPoint<T>(scale*x, scale*y);
  }

  float length() const {
    return sqrt(x*x + y*y);
  }
};

template <typename T>
inline TPoint<T> operator*(const T& scale, const TPoint<T>& point) {
  return TPoint<T>(scale*point.x, scale*point.y);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& out, const TPoint<T>& point) {
  return out << "TPoint(" << point.x << ", " << point.y << ")";
}

typedef TPoint<int> Point;
typedef TPoint<double> Position;

} // namespace skishore

#endif  // __SKISHORE_POINT_H__
