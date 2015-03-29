#include "gen/util.h"

using std::vector;

namespace babel {
namespace gen {

vector<vector<bool>> Construct2DArray(const Point& size, bool value) {
  return vector<vector<bool>>(size.x, vector<bool>(size.y, value));
}

}  // namespace gen
}  // namespace babel
