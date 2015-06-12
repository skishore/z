class Constants
  @moves = {W: [0, -1], A: [-1, 0], S: [0, 1], D: [1, 0]}
  @grid_in_pixels = 16
  @twips_per_pixel = 1024
  @grid = @grid_in_pixels*@twips_per_pixel
  @speed = 0.12*@grid

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel

  @to_square = (position) ->
    new Point (Math.round position.x/@grid), (Math.round position.y/@grid)


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
    @element.append @renderer.view

    @context = new PIXI.Stage 0x00000000
    @container = new PIXI.DisplayObjectContainer
    @context.addChild @container

    do @_initialize_stats

    assets_to_load = ['tileset.json', 'sprites.json']
    loader = new PIXI.AssetLoader assets_to_load
    loader.onComplete = @_on_assets_loaded.bind @
    do loader.load

  _initialize_stats: ->
    @stats = new PIXI.Stats
    $('body').append @stats.domElement
    $(@stats.domElement).css {position: 'fixed', top: 0, left: 0}

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
    @position = start.scale Constants.grid

  move: (vector) ->
    vector = @_check_squares vector
    @position = Point.sum @position, vector

  _check_squares: (move) ->
    move = new Point (Math.round move.x), (Math.round move.y)
    if do move.zero
      return move

    half_grid = Math.ceil 0.5*Constants.grid
    tolerance = Math.ceil 0.2*Constants.grid

    overlap = new Point (@_gmod @position.x), (@_gmod @position.y)
    overlap.x -= if overlap.x < half_grid then 0 else Constants.grid
    overlap.y -= if overlap.y < half_grid then 0 else Constants.grid

    square = Point.difference @position, overlap
    assert (square.x % Constants.grid == 0) and (square.y % Constants.grid == 0)
    square.x /= Constants.grid
    square.y /= Constants.grid

    offset = new Point 0, 0
    collided = false
    speed = Math.floor (do move.length)/2

    # Check if we cross a horizontal grid boundary going up or down.
    if move.y < 0 and (@_gmod @position.y + tolerance) < -move.y
      offset.y = -1
    else if move.y > 0 and (@_gmod -@position.y) < move.y
      offset.y = 1
    # If we cross a horizontal boundary, check that the next square is open.
    if offset.y != 0
      offset.x = if overlap.x > 0 then 1 else -1
      if not @_check_square new Point square.x, square.y + offset.y
        collided = true
      else if (Math.abs overlap.x) > tolerance and \
           not @_check_square Point.sum square, offset
        collided = true
        if (Math.abs overlap.x) <= half_grid and offset.x*move.x <= 0
          move.x = -offset.x*speed
      if collided
        if offset.y < 0
          move.y = Constants.grid - tolerance - @_gmod @position.y
        else
          move.y = @_gmod -@position.y
    # Run similar checks for crossing a vertical boundary going right or left.
    offset.x = 0
    if move.x < 0 and (@_gmod @position.x + tolerance) < -move.x
      offset.x = -1
    else if move.x > 0 and (@_gmod tolerance - @position.x) < move.x
      offset.x = 1
    if offset.x != 0
      # If we've crossed a horizontal boundary (which is true iff offset.y != 0
      # and collided is false) then run an extra check in the x direction.
      collided = offset.y != 0 and not collided and \
                 not @_check_square Point.sum square, offset
      offset.y = if overlap.y > 0 then 1 else -1
      if not @_check_square new Point square.x + offset.x, square.y
        collided = true
      else if (overlap.y > 0 or -overlap.y > tolerance) and
              not @_check_square Point.sum square, offset
        collided = true
        if (Math.abs overlap.y) <= half_grid and offset.y*move.y <= 0
          # Check that we have space to shove away in the y direction.
          # We skip this check when shoving in the x direction because the
          # full x check is after the y check.
          move.y = if (@_check_square new Point square.x, square.y + offset.y) \
                      then -offset.y*speed else 0
      if collided
        if offset.x < 0
          move.x = Constants.grid - tolerance - @_gmod @position.x
        else
          move.x = tolerance - @_gmod @position.x
    move

  _check_square: (square) ->
    (@stage.map.get_tile square) == '.'

  _gmod: (value) ->
    result = value % Constants.grid
    if result >= 0 then result else result + Constants.grid


class Stage
  constructor: ->
    @input = new Input
    @map = new Map new Point 16, 9
    @player = new Sprite @, do @map.get_starting_square
    @graphics = new Graphics @, $('.surface')

  loop: ->
    do @graphics.stats.begin
    do @update
    do @graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @graphics.stats.end

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
