#ifndef __BABEL_BASE_UTIL_H__
#define __BABEL_BASE_UTIL_H__

#include <string>
#include <vector>

#include "util.h"

namespace babel {

std::string IntToString(int value);

std::string DoubleToString(double value, int precision);

template<typename T>
std::vector<T> Concatenate(const std::vector<std::vector<T>>& nested_list) {
  std::vector<T> result;
  for (const std::vector<T>& sublist : nested_list) {
    for (const T& element : sublist) {
      result.push_back(element);
    }
  }
  return result;
}

}  // namespace babel

#endif  // __BABEL_BASE_UTIL_H__
