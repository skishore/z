#ifndef __BABEL_INTERFACE_TRANSLITERATION_MULTIPLE_CHOICE_GAME_H__
#define __BABEL_INTERFACE_TRANSLITERATION_MULTIPLE_CHOICE_GAME_H__

#include <string>
#include <vector>

#include "interface/Game.h"

namespace babel {
namespace interface {

class TransliterationMultipleChoiceGame : public Game {
 public:
  TransliterationMultipleChoiceGame();

  virtual DialogResult Consume(char ch) override;

  virtual bool Active() const override;
  virtual void Draw(render::DialogRenderer* renderer) const override;

  virtual GameResult GetResult() const override;

 private:
  std::string question_;
  std::vector<std::string> answers_;
  std::vector<bool> attempted_;
  int correct_answer_;
  int errors_ = 0;
};

}  // namespace interface
}  // namespace babel

#endif  // __BABEL_INTERFACE_TRANSLITERATION_MULTIPLE_CHOICE_GAME_H__
