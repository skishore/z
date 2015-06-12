class Constants
  @moves = {W: [0, -1], A: [-1, 0], S: [0, 1], D: [1, 0]}
  @grid_in_pixels = 16
  @twips_per_pixel = 1024
  @grid = @grid_in_pixels*@twips_per_pixel
  @speed = 0.125*@grid

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel


class Graphics
  constructor: (@stage, @element, callback) ->
    @size = @stage.map.size.scale Constants.grid_in_pixels
    @scale = 2

    # TODO(skishore): The number of tiles and sprites should be read from JSON.
    @num_tiles = 8
    @num_sprites = 3
    @tile_textures = []
    @sprite_textures = []
    @tiles = []

    @renderer = PIXI.autoDetectRenderer @size.x, @size.y
    @renderer.view.style.width = "#{Math.floor @scale*@size.x}px"
    @renderer.view.style.height = "#{Math.floor @scale*@size.y}px"
    @element.append $ @renderer.view

    @context = new PIXI.Stage 0x00000000
    @container = new PIXI.DisplayObjectContainer
    @context.addChild @container

    assets_to_load = ['tileset.json', 'sprites.json']
    loader = new PIXI.AssetLoader assets_to_load
    loader.onComplete = @_on_assets_loaded.bind @
    do loader.load

  _on_assets_loaded: ->
    for i in [0...@num_tiles]
      @tile_textures.push new PIXI.Texture.fromFrame "tile#{i}.png"
    for i in [0...@num_sprites]
      @sprite_textures.push new PIXI.Texture.fromFrame "sprite#{i}.png"
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        type = if (@stage.map.get_tile new Point x, y) == '.' then 0 else 4
        tile = new PIXI.Sprite @tile_textures[type]
        tile.x = Constants.grid_in_pixels*x
        tile.y = Constants.grid_in_pixels*y
        @tiles.push tile
        @container.addChild tile
    @player = new PIXI.Sprite @sprite_textures[0]
    @container.addChild @player
    do @stage.loop.bind @stage

  draw: ->
    @player.x = Constants.to_pixels @stage.player.position.x
    @player.y = Constants.to_pixels @stage.player.position.y
    @renderer.render @context


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


class Map
  constructor: (@size) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = (do @_get_random_tile for i in [0...@size.x*@size.y])
    @_tiles[0] = '.'

  get_starting_square: ->
    new Point 0, 0

  get_tile: (square) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      return @_tiles[square.x*@size.y + square.y]
    '#'

  _get_random_tile: ->
    if (do Math.random) < 0.2 then '#' else '.'


class Sprite
  constructor: (@stage, start) ->
    @color = 'black'
    @position = start.scale Constants.twips_per_pixel

  move: (vector) ->
    x = @position.x + Math.round vector.x
    y = @position.y + Math.round vector.y
    @position.x = Math.max x, 0
    @position.y = Math.max y, 0


class Stage
  constructor: ->
    @input = new Input
    @map = new Map new Point 16, 9
    @player = new Sprite @, do @map.get_starting_square
    @graphics = new Graphics @, $('.surface')

  loop: ->
    do @update
    do @graphics.draw
    window.requestAnimationFrame @loop.bind @

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
