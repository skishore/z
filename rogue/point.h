#ifndef ROGUE_POINT_H__
#define ROGUE_POINT_H__

struct Point {
 public:
  Point() : x(0), y(0) {}
  Point(int nx, int ny) : x(nx), y(ny) {}

  int x;
  int y;
};

#endif  // ROGUE_POINT_H__
