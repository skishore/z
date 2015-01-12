#ifndef __BABEL_TIMING_H__
#define __BABEL_TIMING_H__

#include <string>

namespace babel {

typedef long long tick;

tick GetCurrentTick();

// All calls to StartTimer have to be followed by a call to EndTimer.
void StartTimer(const std::string& name);
void EndTimer();

}  // namespace babel

#endif  // __BABEL_TIMING_H__
