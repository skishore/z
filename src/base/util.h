#ifndef __BABEL_BASE_UTIL_H__
#define __BABEL_BASE_UTIL_H__

#include <map>
#include <string>
#include <vector>

#include "base/debug.h"

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

template<typename T> std::map<T,T> Invert(const std::map<T,T>& original) {
  std::map<T,T> result;
  for (const auto& pair : original) {
    ASSERT(result.find(pair.second) == result.end());
    result[pair.second] = pair.first;
  }
  return result;
}

}  // namespace babel

#endif  // __BABEL_BASE_UTIL_H__
