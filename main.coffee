class Constants
  @moves = {W: [0, -1], A: [-1, 0], S: [0, 1], D: [1, 0]}
  @grid_in_pixels = 16
  @twips_per_pixel = 1024
  @grid = @grid_in_pixels*@twips_per_pixel
  # Constants for walking states.
  @player_speed = 0.1*@grid
  @enemy_speed = 0.05*@grid
  @walking_animation_frames = 6
  # Constants for the jumping state.
  @jump_height = 1.0*@grid
  @jump_length = 3.0*@grid
  @jump_speed = 1.2*@player_speed
  # Constants for the knockback state.
  @knockback_length = 1.5*@grid
  @knockback_speed = 3*@player_speed
  @knockback_frames = Math.floor @knockback_length/@knockback_speed
  @invulnerability_animation_frames = 6
  @invulnerability_frames = @knockback_frames + 15

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel


class Direction
  @UP = 'up'
  @RIGHT = 'right'
  @DOWN = 'down'
  @LEFT = 'left'

  @UNIT_VECTOR = {up: [0, -1], right: [1, 0], down: [0, 1], left: [-1, 0]}

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

    # TODO(skishore): The number of tiles should be read from JSON.
    @num_tiles = 8
    @sprites = {}
    @sprite_index = 0
    @tile_textures = []
    @tiles = []

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y
    @element.append @renderer.view

    @context = new PIXI.Stage 0x00000000
    @map_container = do @_add_container
    @sprite_container = do @_add_container
    do @_initialize_stats

    assets_to_load = ['enemies.json', 'player.json', 'tileset.json']
    loader = new PIXI.AssetLoader assets_to_load
    loader.onComplete = @_on_assets_loaded.bind @
    do loader.load

  _add_container: ->
    container = new PIXI.DisplayObjectContainer
    container.scale = new PIXI.Point @scale, @scale
    @context.addChild container
    container

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
        @map_container.addChild tile
    do @stage.loop.bind @stage

  draw: ->
    drawn = {}
    for sprite in @stage.sprites
      @_draw_sprite sprite
      drawn[sprite._pixi_id] = true
    ids_to_remove = (id for id of @sprites when not drawn[id])
    for id in ids_to_remove
      delete @sprites[id]
    @sprite_container.children.sort (a, b) -> Math.sign a.y - b.y
    @renderer.render @context

  _draw_sprite: (sprite) ->
    if not sprite._pixi_id?
      pixi = new PIXI.Sprite
      @sprite_container.addChild pixi
      sprite._pixi_id = @sprite_index
      @sprites[@sprite_index] = pixi
      @sprite_index += 1
    texture_name = "#{sprite.image}-#{sprite.frame}-#{sprite.direction}.png"
    y_offset = if sprite._pixi_y_offset? then sprite._pixi_y_offset else 0
    pixi = @sprites[sprite._pixi_id]
    pixi.x = Constants.to_pixels sprite.position.x
    pixi.y = Constants.to_pixels sprite.position.y + y_offset
    pixi.setTexture PIXI.Texture.fromFrame texture_name
    if sprite.invulnerability_frames == 0
      pixi.filters = null
    else
      period = Constants.invulnerability_animation_frames
      pixi.filters = if sprite.invulnerability_frames % (2*period) <= period \
                     then [new PIXI.InvertFilter] else null


class Input
  constructor: ->
    window.onkeydown = @_onkeydown.bind @
    window.onkeyup = @_onkeyup.bind @
    @_blocked = {}
    @_pressed = {}

  block_key: (key) ->
    @_blocked[key] = true

  get_keys_pressed: ->
    _.omit @_pressed, _.keys @_blocked

  _get_key: (e) ->
    String.fromCharCode e.which

  _onkeydown: (e) ->
    @_pressed[@_get_key e] = true

  _onkeyup: (e) ->
    key = @_get_key e
    delete @_blocked[key]
    delete @_pressed[key]


class Map
  constructor: (@size) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = (do @_get_random_tile for i in [0...@size.x*@size.y])
    @_tiles[0] = '.'

  get_random_free_square: ->
    result = new Point -1, -1
    while (@get_tile result) == '#'
      result.x = _.random (@size.x - 1)
      result.y = _.random (@size.y - 1)
    result

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
    @invulnerability_frames = 0
    @position = start.scale Constants.grid
    @square = start
    @set_state state

  collides: (sprite) ->
    grid = Constants.grid
    (not (@position.x + grid <= sprite.position.x or
          sprite.position.x + grid <= @position.x)) and
    (not (@position.y + grid <= sprite.position.y or
          sprite.position.y + grid <= @position.y))

  collides_with_any: ->
    for sprite in @stage.sprites
      if sprite != @ and @collides sprite
        return true
    false

  get_free_direction: ->
    options = []
    for key, move of Constants.moves
      square = new Point @square.x + move[0], @square.y + move[1]
      if @_check_square square
        options.push key
    if options.length == 0
      options = _.keys Constants.moves
    Constants.moves[_.sample options]

  move: (vector) ->
    vector = @_check_squares vector
    if not do vector.zero
      @position = Point.sum @position, vector
      [@square, _] = do @_get_square_and_overlap
    vector

  set_state: (state) ->
    @state?.on_exit?()
    @state = state
    @state.sprite = @
    @state.stage = @stage
    @state.on_enter?()

  update: (keys) ->
    if @invulnerability_frames > 0
      @invulnerability_frames -= 1
    @state.update keys

  _check_squares: (move) ->
    move = new Point (Math.round move.x), (Math.round move.y)
    if do move.zero
      return move

    half_grid = Math.ceil 0.5*Constants.grid
    tolerance = Math.ceil 0.2*Constants.grid

    # Sprites may not move faster than our collision detection can handle.
    # If this restriction becomes a problem, we can get around it by breaking
    # the move up into several steps.
    speed = Math.floor do move.length
    assert speed < half_grid

    [square, overlap] = do @_get_square_and_overlap
    offset = new Point 0, 0
    collided = false

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

  _get_square_and_overlap: ->
    # We first compute an overlap, which is a point (x, y) where each coordinate
    # lies in the interval [-half_grid, half_grid). The overlap is the
    # "remainder" of our position with respect to the grid.
    half_grid = Math.ceil 0.5*Constants.grid
    overlap = new Point (@_gmod @position.x), (@_gmod @position.y)
    overlap.x -= if overlap.x < half_grid then 0 else Constants.grid
    overlap.y -= if overlap.y < half_grid then 0 else Constants.grid
    # By subtracting the overlap from our position we round it to a grid square.
    square = Point.difference @position, overlap
    assert (square.x % Constants.grid == 0) and (square.y % Constants.grid == 0)
    square.x /= Constants.grid
    square.y /= Constants.grid
    [square, overlap]

  _gmod: (value) ->
    result = value % Constants.grid
    if result >= 0 then result else result + Constants.grid


_get_move = (keys, speed) ->
  move = new Point 0, 0
  for key of keys
    if key of Constants.moves
      [x, y] = Constants.moves[key]
      move.x += x
      move.y += y
  if (do move.zero) then move else move.scale_to speed

_move_sprite = (attempt) ->
  move = @sprite.move attempt
  if not do move.zero and @_period?
    period = Math.floor @_period*Constants.player_speed/(do move.length)
    period = Math.max period, 1
    if @_anim_num % (2*period) >= period
      animate = true
    @_anim_num = (@_anim_num + 1) % (2*period)
  @sprite.direction = Direction.get_move_direction attempt, @sprite.direction
  @sprite.frame = if animate then 'walking' else 'standing'

_switch_state = (sprite, new_state) ->
  sprite.set_state new_state
  do sprite.state.update


class JumpingState
  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil Constants.jump_length/Constants.jump_speed

  on_exit: ->
    delete @sprite._pixi_y_offset

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return _switch_state @sprite, new WalkingState
    keys = do @sprite.stage.input.get_keys_pressed
    _move_sprite.call @, _get_move keys, Constants.jump_speed
    @sprite.frame = "jumping#{Math.floor 3*(@_cur_frame - 1)/@_max_frame}"
    @sprite._pixi_y_offset = do @_get_y_offset

  _get_y_offset: ->
    arc_position = (Math.pow 2*@_cur_frame/@_max_frame - 1, 2) - 1
    Math.floor Constants.jump_height*arc_position


class KnockbackState
  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil Constants.knockback_length/Constants.knockback_speed

  on_enter: ->
    @_direction = @sprite.direction
    @sprite.invulnerability_frames = Constants.invulnerability_frames

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return _switch_state @sprite, new WalkingState
    [x, y] = Direction.UNIT_VECTOR[@_direction]
    _move_sprite.call @, (new Point x, y).scale_to -Constants.knockback_speed
    @sprite.direction = @_direction


class PausedState
  constructor: ->
    speed = Constants.enemy_speed
    base_steps = Math.floor Constants.grid/speed
    @_steps = _.random -base_steps, base_steps

  update: ->
    @_steps -= 1
    if @_steps < 0
      return _switch_state @sprite, new RandomWalkState
    @sprite.frame = 'standing'


class RandomWalkState
  constructor: ->
    @_anim_num = 0
    @_period = Constants.walking_animation_frames

  on_enter: ->
    speed = Constants.enemy_speed
    base_steps = Math.floor Constants.grid/speed
    [x, y] = do @sprite.get_free_direction
    @_move = (new Point x, y).scale_to speed
    @_steps = _.random 1, 4*base_steps

  update: ->
    @_steps -= 1
    if @_steps < 0
      return _switch_state @sprite, new PausedState
    _move_sprite.call @, @_move


class WalkingState
  constructor: ->
    @_anim_num = 0
    @_period = Constants.walking_animation_frames

  update: ->
    if @sprite.invulnerability_frames == 0 and do @sprite.collides_with_any
      return _switch_state @sprite, new KnockbackState
    keys = do @sprite.stage.input.get_keys_pressed
    if keys.J?
      @sprite.stage.input.block_key 'J'
      return _switch_state @sprite, new JumpingState
    _move_sprite.call @, _get_move keys, Constants.player_speed



class Stage
  constructor: ->
    @input = new Input
    @map = new Map new Point 16, 9
    @player = do @_construct_player
    @sprites = [@player].concat (do @_construct_enemy for i in [0...4])
    @_graphics = new Graphics @, $('.surface')

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    do sprite.update for sprite in @sprites

  _construct_enemy: ->
    new Sprite @, 'enemy', (do @map.get_random_free_square), new PausedState

  _construct_player: ->
    new Sprite @, 'player', (do @map.get_starting_square), new WalkingState


Meteor.startup (-> stage = new Stage) if Meteor.isClient
