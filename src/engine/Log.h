#ifndef __BABEL_ENGINE_LOG_H__
#define __BABEL_ENGINE_LOG_H__

#include <deque>
#include <string>
#include <vector>

namespace babel {
namespace engine {

class Log {
 public:
  void AddLine(const std::string& line);
  void Flush(bool changed);
  void Open();

  std::string GetCurrentLine() const;
  std::vector<std::string> GetLastLines(int n) const;

  // Returns true if a line was added the last time the log was coalesced.
  bool IsFresh() const;

 private:
  std::deque<std::string> lines_;
  std::deque<std::string> new_lines_;
  bool fresh_ = false;
  bool open_ = false;
};

}  // namespace engine 
}  // namespace babel

#endif  // __BABEL_ENGINE_LOG_H__
