window.onload = ->
  $('body').append $('<div>').addClass 'centered'

  [cols, rows] = [80, 24]
  engine = new Engine cols, rows
  graphics = new Graphics $('.centered'), do engine.get_view

  graphics.target.attr 'tabindex', 1
  graphics.target.keydown (e) ->
    char = String.fromCharCode e.which
    if not e.shiftKey
      char = do char.toLowerCase
    if engine.do_command char
      graphics.redraw do engine.get_view
  do graphics.target.focus
