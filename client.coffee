_twips = 1024

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
    @size = do size.clone

  move: (vector) ->
    x = @position.x + Math.round vector.x
    y = @position.y + Math.round vector.y
    @position.x = Math.min (Math.max x, 0), @surface.x - @size.x
    @position.y = Math.min (Math.max y, 0), @surface.y - @size.y

  redraw: ->
    if not @_last_css?
      @_element = $('<div>').addClass 'sprite'
      $('.surface').append @_element
      @_last_css = {}
    css = do @_get_css
    update = {}
    for key of css
      if @_last_css[key] != css[key]
        update[key] = css[key]
    if (_.keys update).length > 0
      @_element.css update
      @_last_css = css

  _get_css: ->
    'background-color': @color
    left: Math.floor @position.x/_twips
    top: Math.floor @position.y/_twips
    width: Math.floor @size.x/_twips
    height: Math.floor @size.y/_twips


class Enemy extends Sprite
  constructor: (surface, size) ->
    super surface, size
    @color = 'red'

  get_move: (player, speed) ->
    move = new Point (_.random -speed, speed), (_.random -speed, speed)
    approach = Point.difference player.position, @position
    if not do approach.zero
      approach = approach.scale_to speed/4
    #Point.sum move, approach
    approach


class Game
  constructor: ->
    # Set up constants.
    @keys = {w: [0, -1], a: [-1, 0], s: [0, 1], d: [1, 0]}
    @num_enemies = 8
    @speed = 4*_twips
    @sprite_size = new Point 32*_twips, 32*_twips
    @surface = new Point 640*_twips, 360*_twips
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
      @player.move move.scale_to @speed
    for enemy in @enemies
      move = enemy.get_move @player, @speed
      move = @_apply_kinematic_constraints enemy, move
      enemy.move move

  _apply_kinematic_constraints: (enemy, move) ->
    for other in @enemies
      if other == enemy
        continue
      diff = Point.difference enemy.position, other.position
      if do diff.zero
        continue
      diff = diff.scale_to (enemy.size.x*_twips)/(do diff.length)/4
      move = Point.sum move, diff
    move

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
