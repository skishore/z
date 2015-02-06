// Abstract class used to implement the main command dialog as well as all
// the sub-dialogs used to play semantic games. This header forward-declares
// all of its dependencies, so it is safe to import it in the render, engine,
// and ui namespaces.
//
#ifndef __BABEL_INTERFACE_DIALOG_H__
#define __BABEL_INTERFACE_DIALOG_H__

struct SDL_Renderer;

namespace babel {

namespace engine {
class Action;
}  // namespace engine

namespace render {
class DialogRenderer;
}  // namespace render

namespace interface {

class Dialog {
 public:
  virtual ~Dialog() {};

  // Used when the player makes a move. Renders this dialog inactive.
  virtual void Clear() = 0;

  // Used for input. Returns true if the input was consumed.
  // If this method returns a non-null action, the caller takes ownership of it
  // and should pass it to the engine's Update method.
  virtual bool Consume(char ch, engine::Action** action, bool* redraw) = 0;

  // Const methods used to render the dialog in the UI.
  virtual bool Active() const = 0;
  virtual void Draw(render::DialogRenderer* renderer) const = 0;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_INTERFACE_DIALOG_H__
