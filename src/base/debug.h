#ifndef __BABEL_BASE_DEBUG_H__
#define __BABEL_BASE_DEBUG_H__

#include <iostream>
#include <stdlib.h>

namespace babel {

#ifdef NDEBUG

#define ASSERT(x) (void)(x)
#define DEBUG(x)

#else  // NDEBUG

#define ASSERT(x) do { \
  if (!(x)) { \
    DEBUG("ASSERTION FAILED: " << #x); \
    exit(-1); \
  } \
} while(false)

#define DEBUG(x) do { \
  std::cout << "[" << __FILE__ << ":" << __LINE__ << "] " << x << std::endl; \
} while(false)

#endif  // NDEBUG

void RegisterCrashHandlers(const char* binary);

}  // namespace babel

#endif  // __BABEL_BASE_DEBUG_H__
