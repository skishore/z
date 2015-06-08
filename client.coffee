_get_key = (e) ->
  key = String.fromCharCode e.which
  if not e.shiftKey
    key = do key.toLowerCase
  key


class Sprite
  constructor: (position, size) ->
    @position = Point.copy position
    @size = Point.copy size

  move: (surface, vector) ->
    x = @position.x + vector.x
    y = @position.y + vector.y
    @position.x = Math.min (Math.max x, 0), surface.x - @size.x
    @position.y = Math.min (Math.max y, 0), surface.y - @size.y

  redraw: ->
    if not @_last_position?
      @_element = $('<div>').addClass 'sprite'
      @_element.css {width: @size.x, height: @size.y}
      $('.surface').append @_element
      @_last_position = new Point null, null
    if not @_last_position.equals @position
      @_element.css {left: @position.x, top: @position.y}
      @_last_position = Point.copy @position
      

class Game
  constructor: ->
    # Set up constants.
    @keys = {w: [0, -1], a: [-1, 0], s: [0, 1], d: [1, 0]}
    @sprite_size = new Point 16, 16
    @surface = new Point 640, 360
    # Set up instance variables and handlers.
    @player = new Sprite @sprite_size, @sprite_size
    @pressed = {}
    window.onkeydown = @onkeydown.bind @
    window.onkeyup = @onkeyup.bind @
    window.requestAnimationFrame @loop.bind @

  loop: ->
    do @update
    do @redraw
    window.requestAnimationFrame @loop.bind @

  redraw: ->
    do @player.redraw

  update: ->
    move = new Point 0, 0
    for key of @pressed
      [x, y] = @keys[key]
      move.x += x
      move.y += y
    if not do move.zero
      @player.move @surface, move.scale_to 4

  onkeydown: (e) ->
    key = _get_key e
    if key not of @keys
      return
    @pressed[key] = true

  onkeyup: (e) ->
    key = _get_key e
    if key not of @keys
      return
    delete @pressed[key]


if Meteor.isClient
  Meteor.startup ->
    game = new Game
