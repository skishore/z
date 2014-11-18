#ifndef __SKISHORE_DEBUG_H__
#define __SKISHORE_DEBUG_H__

#include <iostream>
#include <stdlib.h>

namespace skishore {

#ifdef NDEBUG

#define ASSERT(x, y)
#define DEBUG(x)
#define USE_FOR_DEBUG(x) (void)(x)

#else  // NDEBUG

#define ASSERT(x, y) do { \
  if (!(x)) { \
    DEBUG("ASSERTION FAILED: " << y); \
    exit(-1); \
  } \
} while(false)

#define DEBUG(x) do { \
  std::cout << "[" << __FILE__ << ":" << __LINE__ << "] " << x << std::endl; \
} while(false)

#define USE_FOR_DEBUG(x)

#endif  // NDEBUG

}  // namespace skishore

#endif  // __SKISHORE_DEBUG_H__
