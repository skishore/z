class Constants
  @moves = {W: [0, -1], A: [-1, 0], S: [0, 1], D: [1, 0]}
  @twips_per_pixel = 1024
  @speed = 4*@twips_per_pixel
  @sprite_size = (new Point 32, 32).scale @twips_per_pixel
  @surface = (new Point 640, 360).scale @twips_per_pixel

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel


class Input
  constructor: ->
    window.onkeydown = @_onkeydown.bind @
    window.onkeyup = @_onkeyup.bind @
    @_pressed = {}

  get_keys_pressed: ->
    _.clone @_pressed

  _get_key: (e) ->
    String.fromCharCode e.which

  _onkeydown: (e) ->
    @_pressed[@_get_key e] = true

  _onkeyup: (e) ->
    delete @_pressed[@_get_key e]


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
    left: Constants.to_pixels @position.x
    top: Constants.to_pixels @position.y
    width: Constants.to_pixels @size.x
    height: Constants.to_pixels @size.y


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
    @input = new Input
    @player = new Sprite Constants.surface, Constants.sprite_size
    window.requestAnimationFrame @loop.bind @

  loop: ->
    do @update
    do @redraw
    window.requestAnimationFrame @loop.bind @

  redraw: ->
    do @player.redraw

  update: ->
    move = new Point 0, 0
    for key of do @input.get_keys_pressed
      if key of Constants.moves
        [x, y] = Constants.moves[key]
        move.x += x
        move.y += y
    if not do move.zero
      @player.move move.scale_to Constants.speed


if Meteor.isClient
  Meteor.startup ->
    game = new Game
