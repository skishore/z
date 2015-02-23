#ifndef __BABEL_INTERFACE_TRANSLITERATION_SHORT_ANSWER_GAME_H__
#define __BABEL_INTERFACE_TRANSLITERATION_SHORT_ANSWER_GAME_H__

#include <string>
#include <vector>

#include "interface/Game.h"

namespace babel {
namespace interface {

class TransliterationShortAnswerGame : public Game {
 public:
  TransliterationShortAnswerGame();

  virtual DialogResult Consume(char ch) override;

  virtual bool Active() const override;
  virtual void Draw(render::DialogRenderer* renderer) const override;

  virtual GameResult GetResult() const override;

 private:
  void Advance();

  std::vector<std::string> segments_;
  std::vector<std::string> answers_;
  std::vector<std::string> entries_;
  std::vector<bool> guides_;
  int index_ = 0;
  int length_ = 0;
  int errors_ = 0;
  // The number of frames to pause when the game is complete.
  int pause_ = 4;
};

}  // namespace interface
}  // namespace babel

#endif  // __BABEL_INTERFACE_TRANSLITERATION_SHORT_ANSWER_GAME_H__
