#ifndef BABEL_LOG_H__
#define BABEL_LOG_H__

#include <deque>
#include <string>
#include <vector>

namespace babel {

class Log {
 public:
  void AddLine(const std::string& line);
  void Coalesce();

  std::vector<std::string> GetLastLines(int n) const;

  // Returns true if a line was added the last time the log was coalesced.
  bool IsFresh() const;

 private:
  std::deque<std::string> lines_;
  std::deque<std::string> new_lines_;
  bool fresh_ = false;
};

}  // namespace babel

#endif  // BABEL_LOG_H__