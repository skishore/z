#ifndef __SKISHORE_DEBUG_H__
#define __SKISHORE_DEBUG_H__

#include <iostream>

namespace skishore {

#define DEBUG(x) do { \
  std::cout << "[" << __FILE__ << ":" << __LINE__ << "] " << x << std::endl; \
} while(false);

}  // namespace skishore

#endif  // __SKISHORE_DEBUG_H__
