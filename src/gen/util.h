#ifndef __BABEL_GEN_UTIL_H__
#define __BABEL_GEN_UTIL_H__

#include <vector>

#include "base/point.h"

namespace babel {
namespace gen {

std::vector<std::vector<bool>> Construct2DArray(const Point& size, bool value);

}  // namespace gen
}  // namespace babel

#endif  // __BABEL_GEN_UTIL_H__
