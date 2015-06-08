_get_key = (e) ->
  key = String.fromCharCode e.which
  if not e.shiftKey
    key = do key.toLowerCase
  key


class Sprite
  constructor: (@surface, size) ->
    @color = 'black'
    @position = new Point (_.random @surface.x - size.x), \
                          (_.random @surface.y - size.y)
    @size = Point.copy size

  move: (vector) ->
    x = @position.x + vector.x
    y = @position.y + vector.y
    @position.x = Math.min (Math.max x, 0), @surface.x - @size.x
    @position.y = Math.min (Math.max y, 0), @surface.y - @size.y

  redraw: ->
    if not @_last_position?
      @_element = $('<div>').addClass 'sprite'
      @_element.css 'background-color', @color
      @_element.css {width: @size.x, height: @size.y}
      $('.surface').append @_element
      @_last_position = new Point null, null
    if not @_last_position.equals @position
      @_element.css {left: @position.x, top: @position.y}
      @_last_position = Point.copy @position


class Enemy extends Sprite
  constructor: (surface, size) ->
    super surface, size
    @color = 'red'

  get_move: (player) ->
    move = new Point (_.random -4, 4), (_.random -4, 4)
    move.x += (if player.position.x > @position.x then 1 else -1)
    move.y += (if player.position.y > @position.y then 1 else -1)
    move


class Game
  constructor: ->
    # Set up constants.
    @keys = {w: [0, -1], a: [-1, 0], s: [0, 1], d: [1, 0]}
    @num_enemies = 8
    @sprite_size = new Point 16, 16
    @surface = new Point 640, 360
    # Set up instance variables.
    @enemies = (new Enemy @surface, @sprite_size for i in [0...@num_enemies])
    @player = new Sprite @surface, @sprite_size
    @pressed = {}
    @sprites = [@player].concat @enemies
    # Set up handlers.
    window.onkeydown = @onkeydown.bind @
    window.onkeyup = @onkeyup.bind @
    window.requestAnimationFrame @loop.bind @

  loop: ->
    do @update
    do @redraw
    window.requestAnimationFrame @loop.bind @

  redraw: ->
    for sprite in @sprites
      do sprite.redraw

  update: ->
    move = new Point 0, 0
    for key of @pressed
      [x, y] = @keys[key]
      move.x += x
      move.y += y
    if not do move.zero
      @player.move move.scale_to 4
    for enemy in @enemies
      enemy.move enemy.get_move @player

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
