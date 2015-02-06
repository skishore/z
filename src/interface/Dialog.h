// Abstract class used to implement the main command dialog as well as all
// the sub-dialogs used to play semantic games. This header forward-declares
// all of its dependencies, so it is safe to import it in the render, engine,
// and ui namespaces.
//
#ifndef __BABEL_INTERFACE_DIALOG_H__
#define __BABEL_INTERFACE_DIALOG_H__

namespace babel {

namespace engine {
class Action;
}  // namespace engine

namespace render {
class DialogRenderer;
}  // namespace render

namespace interface {

// success is set to true if the input is consumed.
// update is set to true if the engine should be updated, in which case action
// may be null or non-null. If non-null, the engine will take ownership of it.
struct DialogResult {
  bool success = false;
  bool redraw = false;
  bool update = false;
  engine::Action* action = nullptr;
};

class Dialog {
 public:
  virtual ~Dialog() {};

  // Methods used to update the dialog and possibly close it.
  virtual void Clear() {};
  virtual DialogResult Consume(char ch) = 0;

  // Const methods used to render the dialog in the UI.
  virtual bool Active() const = 0;
  virtual void Draw(render::DialogRenderer* renderer) const = 0;
};

}  // namespace dialog
}  // namespace babel

#endif  // __BABEL_INTERFACE_DIALOG_H__
