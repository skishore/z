#ifndef __SKISHORE_DEBUG_H__
#define __SKISHORE_DEBUG_H__

#include <iostream>
#include <stdlib.h>

namespace skishore {

// IMPORTANT: The expression x will be evaluated even if NDEBUG is true!
// However, if NDEBUG is true, y will never be evaluated.
#define ASSERT(x, y) do { \
  if (!(x)) { \
    DEBUG("ASSERTION FAILED: " << (y)); \
    exit(-1); \
  } \
} while(false)

#define DEBUG(x) do { \
  std::cout << "[" << __FILE__ << ":" << __LINE__ << "] " << x << std::endl; \
} while(false)

}  // namespace skishore

#endif  // __SKISHORE_DEBUG_H__
