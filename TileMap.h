#ifndef BABEL_TILE_MAP_H__
#define BABEL_TILE_MAP_H__

#include <vector>

struct TileMap {
 public:
  TileMap(int ncols, int nrows) : cols(ncols), rows(nrows), tiles(ncols) {
    for (int x = 0; x < ncols; x++) {
      for (int y = 0; y < ncols; y++) {
        tiles[x].push_back('\0');
      }
    }
  }

  bool IsSquareBlocked(int x, int y) const {
    if (0 <= x && x < cols && 0 <= y && y < rows) {
      return tiles[x][y] != '.';
    }
    return true;
  }

  int cols;
  int rows;
  std::vector<std::vector<char>> tiles;
};

#endif  // BABEL_TILE_MAP_H__
