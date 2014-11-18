#ifndef __SKISHORE_POINT_H__
#define __SKISHORE_POINT_H__

#include <complex>
#include <iostream>

namespace skishore {

namespace {
// For converting a TPoint of one type to a TPoint of another, we use convert.
template <typename T, typename U> inline T convert(const U& u) {
  return (T)u;
}

// We have special logic for converting from doubles to ints that rounds.
template <> inline int convert(const double& u) {
  return round(u);
}
}  // namespace

// For use when doing floating-point computations with positions.
static const double kZero = 0.001;

template <typename T>
struct TPoint {
  T x, y;

  TPoint<T>(T x0=0, T y0=0) : x(x0), y(y0) {};

  template <typename U>
  TPoint<T>(TPoint<U> other)
      : x(convert<T,U>(other.x)), y(convert<T,U>(other.y)) {};

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

  void operator*=(const T& scale) {
    x *= scale;
    y *= scale;
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

  TPoint<T> operator/(const T& scale) const {
    return TPoint<T>(x/scale, y/scale);
  }

  double length() const {
    return sqrt(x*x + y*y);
  }

  void set_length(double new_length) {
    double old_length = length();
    if (old_length < kZero) {
      return;
    }
    double scale = new_length/old_length;
    x *= scale;
    y *= scale;
  }
};

template <typename T>
inline TPoint<T> operator*(const T& scale, const TPoint<T>& point) {
  return TPoint<T>(scale*point.x, scale*point.y);
}

typedef TPoint<int> Point;
typedef TPoint<double> Position;

inline std::ostream& operator<<(std::ostream& out, const Point& point) {
  return out << "Point(" << point.x << ", " << point.y << ")";
}

inline std::ostream& operator<<(std::ostream& out, const Position& position) {
  // Suppress emscripten's warning about the unused double -> int specialization
  // of the convert function.
  #ifdef EMSCRIPTEN
  Point point(position);
  #endif  // EMSCRIPTEN
  return out << "Position(" << position.x << ", " << position.y << ")";
}

} // namespace skishore

#endif  // __SKISHORE_POINT_H__
