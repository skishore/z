#ifndef __BABEL_INTERFACE_TRANSLITERATION_GAME_H__
#define __BABEL_INTERFACE_TRANSLITERATION_GAME_H__

#include <string>

#include "interface/Dialog.h"

namespace babel {
namespace interface {

class TransliterationGame : public Dialog {
 public:
  TransliterationGame();

  virtual DialogResult Consume(char ch) override;

  virtual bool Active() const override;
  virtual void Draw(render::DialogRenderer* renderer) const override;

 private:
  std::string hindi_;
  std::string english_;
  std::string input_;
};

}  // namespace interface
}  // namespace babel

#endif  // __BABEL_INTERFACE_TRANSLITERATION_GAME_H__
