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


class Graphics
  constructor: (@stage, @element) ->
    @element.css {
      width: Constants.to_pixels @stage.map.size.x*Constants.sprite_size.x
      height: Constants.to_pixels @stage.map.size.y*Constants.sprite_size.y
    }


class Map
  constructor: (@size) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = (do @_get_random_tile for i in [0...@size.x*@size.y])
    @_tiles[0] = '.'

  get_starting_square: ->
    new Point 0, 0

  get_tile: (square) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
     @_tiles[square.x*@size.y + square.y]
    '#'

  _get_random_tile: ->
    if (do Math.random) < 0.2 then '#' else '.'


class Sprite
  constructor: (@stage, start) ->
    @color = 'black'
    @position = start.scale Constants.twips_per_pixel
    @size = do Constants.sprite_size.clone

  move: (vector) ->
    x = @position.x + Math.round vector.x
    y = @position.y + Math.round vector.y
    @position.x = Math.max x, 0
    @position.y = Math.max y, 0

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


class Stage
  constructor: ->
    @input = new Input
    @map = new Map new Point 16, 9
    @player = new Sprite @, do @map.get_starting_square
    @graphics = new Graphics @, $('.surface')
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


Meteor.startup (-> stage = new Stage) if Meteor.isClient
