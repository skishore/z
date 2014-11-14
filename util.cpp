#include <cmath>
#include <string>

#include "util.h"

using std::string;

namespace skishore {

string IntToString(int value) {
  if (value == 0) {
    return "0";
  }
  string result;
  if (value < 0) {
    result = '-' + result;
    value = -value;
  }
  while (value > 0) {
    result = static_cast<char>('0' + (value % 10)) + result;
    value /= 10;
  }
  return result;
}

string DoubleToString(double value, int precision) {
  int base = pow(10, precision);
  string decimal = IntToString((((int)(base*value) % base) + base) % base);
  while (decimal.size() < precision) {
    decimal = '0' + decimal;
  }
  return IntToString((int)value) + "." + decimal;
}

}  // namespace skishore
