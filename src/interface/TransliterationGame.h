#ifndef __BABEL_INTERFACE_TRANSLITERATION_GAME_H__
#define __BABEL_INTERFACE_TRANSLITERATION_GAME_H__

#include <string>
#include <vector>

#include "interface/Dialog.h"

namespace babel {
namespace interface {

class TransliterationGame : public Dialog {
 public:
  TransliterationGame();

  virtual DialogResult Consume(char ch) override;

  virtual bool Active() const override;
  virtual void Draw(render::DialogRenderer* renderer) const override;

  int errors_ = 0;

 private:
  void Advance();

  std::vector<std::string> segments_;
  std::vector<std::string> answers_;
  std::vector<std::string> entries_;
  std::vector<bool> guides_;
  int index_ = 0;
  int length_ = 0;
};

}  // namespace interface
}  // namespace babel

#endif  // __BABEL_INTERFACE_TRANSLITERATION_GAME_H__
