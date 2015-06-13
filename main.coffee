class Constants
  @moves = {W: [0, -1], A: [-1, 0], S: [0, 1], D: [1, 0]}
  @grid_in_pixels = 16
  @twips_per_pixel = 1024
  @grid = @grid_in_pixels*@twips_per_pixel
  @speed = 0.1*@grid

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel


class Direction
  @UP = 'up'
  @RIGHT = 'right'
  @DOWN = 'down'
  @LEFT = 'left'

  @get_move_direction: (move, last_direction) ->
    options = []
    options.push Direction.UP if move.y < 0
    options.push Direction.RIGHT if move.x > 0
    options.push Direction.DOWN if move.y > 0
    options.push Direction.LEFT if move.x < 0
    if options.length > 0 and (_.indexOf options, last_direction) < 0
      return options[0]
    last_direction


class Graphics
  constructor: (@stage, @element, callback) ->
    @scale = 2
    @size = @stage.map.size.scale @scale*Constants.grid_in_pixels

    # TODO(skishore): The number of tiles and sprites should be read from JSON.
    @num_tiles = 8
    @tile_textures = []
    @tiles = []

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y
    @element.append @renderer.view

    @context = new PIXI.Stage 0x00000000
    @container = new PIXI.DisplayObjectContainer
    @container.scale = new PIXI.Point @scale, @scale
    @context.addChild @container

    do @_initialize_stats

    assets_to_load = ['enemies.json', 'player.json', 'tileset.json']
    loader = new PIXI.AssetLoader assets_to_load
    loader.onComplete = @_on_assets_loaded.bind @
    do loader.load

  _initialize_stats: ->
    @stats = new PIXI.Stats
    $('body').append @stats.domElement
    $(@stats.domElement).css {position: 'fixed', top: 0, left: 0}

  _on_assets_loaded: ->
    for i in [0...@num_tiles]
      @tile_textures.push PIXI.Texture.fromFrame "tile#{i}.png"
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        type = if (@stage.map.get_tile new Point x, y) == '.' then 0 else 4
        if type == 0
          type = Math.floor 4*(do Math.random)
        tile = new PIXI.Sprite @tile_textures[type]
        tile.x = Constants.grid_in_pixels*x
        tile.y = Constants.grid_in_pixels*y
        @tiles.push tile
        @container.addChild tile
    @player = new PIXI.Sprite
    @container.addChild @player
    do @stage.loop.bind @stage

  draw: ->
    @_draw_sprite @stage.player, @player
    @renderer.render @context

  _draw_sprite: (sprite, pixi) ->
    pixi.x = Constants.to_pixels sprite.position.x
    pixi.y = Constants.to_pixels sprite.position.y
    texture_name = "#{sprite.image}-#{sprite.direction}-#{sprite.frame}.png"
    pixi.setTexture PIXI.Texture.fromFrame texture_name


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
  constructor: (@stage, @image, start, state) ->
    @direction = Direction.DOWN
    @frame = 'standing'
    @position = start.scale Constants.grid
    @set_state state

  move: (vector) ->
    vector = @_check_squares vector
    @position = Point.sum @position, vector
    vector

  set_state: (state) ->
    @state = state
    @state.sprite = @
    @state.stage = @stage

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
    speed = Math.floor do move.length

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
          shove = Math.min speed, (Math.abs overlap.x) - tolerance
          move.x = -offset.x*shove
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
          shove = Math.min speed, Math.max overlap.y, -overlap.y - tolerance
          move.y = if (@_check_square new Point square.x, square.y + offset.y) \
                      then -offset.y*shove else 0
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


class WalkingState
  @_period = 8

  constructor: ->
    @_anim_num = 0

  update: (keys) ->
    attempt = @_get_move keys
    move = @sprite.move attempt
    if not do move.zero
      period = Math.floor WalkingState._period*Constants.speed/(do move.length)
      period = Math.max period, 1
      if @_anim_num % (2*period) >= period
        animate = true
      @_anim_num = (@_anim_num + 1) % (2*period)
    @sprite.direction = Direction.get_move_direction attempt, @sprite.direction
    @sprite.frame = if animate then 'walking' else 'standing'

  _get_move: (keys) ->
    move = new Point 0, 0
    for key of keys
      if key of Constants.moves
        [x, y] = Constants.moves[key]
        move.x += x
        move.y += y
    if (do move.zero) then move else move.scale_to Constants.speed


class Stage
  constructor: ->
    @_input = new Input
    @map = new Map new Point 16, 9
    @player = do @_construct_player
    @_graphics = new Graphics @, $('.surface')

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    keys = do @_input.get_keys_pressed
    @player.state.update keys

  _construct_player: ->
    new Sprite @, 'player', (do @map.get_starting_square), new WalkingState


Meteor.startup (-> stage = new Stage) if Meteor.isClient
