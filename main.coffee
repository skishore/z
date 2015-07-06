class Constants
  @moves = {w: [0, -1], a: [-1, 0], s: [0, 1], d: [1, 0]}
  @grid_in_pixels = 16
  @twips_per_pixel = 1024
  @grid = @grid_in_pixels*@twips_per_pixel
  # Base constants walking states, used to compute other constants.
  @player_speed = 0.08*@grid
  @enemy_speed = 0.04*@grid
  @walking_animation_frames = 6
  # Constants for the knockback state, used in multiple places.
  @extra_invulnerability_frames = 15
  @invulnerability_animation_frames = 6

  @to_pixels = (twips) ->
    Math.round twips/@twips_per_pixel


class Direction
  @UP = 'up'
  @RIGHT = 'right'
  @DOWN = 'down'
  @LEFT = 'left'

  @OPPOSITE = {up: 'down', right: 'left', down: 'up', left: 'right'}
  @UNIT_VECTOR = {up: [0, -1], right: [1, 0], down: [0, 1], left: [-1, 0]}

  @get_move_direction: (move, last_direction) ->
    options = []
    options.push Direction.UP if move.y < 0
    options.push Direction.RIGHT if move.x > 0
    options.push Direction.DOWN if move.y > 0
    options.push Direction.LEFT if move.x < 0
    if options.length > 0 and (options.indexOf last_direction) < 0
      return options[0]
    last_direction


class Graphics
  constructor: (@stage, @element, callback) ->
    @scale = 2
    @size = @stage.map.size.scale @scale*Constants.grid_in_pixels

    # TODO(skishore): The number of tiles should be read from JSON.
    @num_tiles = 8
    @sprites = {}
    @tile_textures = []
    @tiles = []

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y
    @element.prepend @renderer.view

    @context = new PIXI.Stage 0x00000000
    @map_container = do @_add_container
    @sprite_container = do @_add_container
    do @_initialize_stats

    assets_to_load = ['effects', 'enemies', 'player', 'tileset']
    loader = new PIXI.AssetLoader ("#{asset}.json" for asset in assets_to_load)
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
      @tile_textures.push PIXI.Texture.fromFrame "tile#{i}"
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
      @_draw_sprite sprite._pixi_data, drawn
    ids_to_remove = (id for id of @sprites when not drawn[id])
    for id in ids_to_remove
      @_remove_sprite id
    @sprite_container.children.sort (a, b) -> Math.sign b.z - a.z
    @context.filters = if @stage._pixi_invert \
                       then [new PIXI.InvertFilter] else null
    @renderer.render @context

  _draw_sprite: (sprite, drawn) ->
    pixi = @_get_sprite sprite.id
    pixi.x = Constants.to_pixels sprite.position.x
    pixi.y = Constants.to_pixels sprite.position.y + sprite.y_offset
    pixi.z = -sprite.position.y + Constants.grid*sprite.y_offset
    pixi.setTexture PIXI.Texture.fromFrame sprite.frame
    if sprite.invulnerability_frames == 0
      pixi.filters = null
    else
      period = Constants.invulnerability_animation_frames
      pixi.filters = if sprite.invulnerability_frames % (2*period) < period \
                     then [new PIXI.InvertFilter] else null
    drawn[sprite.id] = true
    @_draw_shadow sprite, drawn
    @_draw_text sprite

  _draw_shadow: (sprite, drawn) ->
    if not sprite.shadow?
      return
    shadow = sprite.shadow
    shadow_id = "#{sprite.id}shadow"
    pixi = @_get_sprite shadow_id
    pixi.x = Constants.to_pixels sprite.position.x + (shadow.x_offset or 0)
    pixi.y = Constants.to_pixels sprite.position.y + (shadow.y_offset or 0)
    pixi.z = -sprite.position.y + (shadow.z_offset or 0)
    pixi.setTexture PIXI.Texture.fromFrame shadow.image
    drawn[shadow_id] = true

  _draw_text: (sprite) ->
    if not sprite.label?
      return
    element = @_get_text sprite.id, sprite.label
    element?.css 'transform', "translateX(-50%) translate(#{
      @scale*Constants.to_pixels sprite.position.x + Constants.grid/2}px, #{
      @scale*((Constants.to_pixels sprite.position.y + Constants.grid) + 1)}px)"

  _get_sprite: (id) ->
    if not @sprites[id]?
      pixi = new PIXI.Sprite
      @sprite_container.addChild pixi
      @sprites[id] = pixi
    @sprites[id]

  _get_text: (id, label) ->
    # TODO(skishore): Use Meteor to display sprite labels instead of this hack.
    selector = "#pixi-text-#{id}.pixi-text"
    if $(selector).length == 0
      element = $("<div id='pixi-text-#{id}'>").addClass 'pixi-text'
      (element.text label.text).addClass label.class
      $('.surface').append element
    $(selector)

  _remove_sprite: (id) ->
    @sprite_container.removeChild @sprites[id]
    do $("#pixi-text-#{id}.pixi-text").remove
    delete @sprites[id]


class Input
  constructor: ->
    window.onkeydown = @_onkeydown.bind @
    window.onkeyup = @_onkeyup.bind @
    @_blocked = {}
    @_pressed = {}

  block_key: (key) ->
    @_blocked[key] = true

  get_keys_pressed: ->
    _.fast_omit @_pressed, @_blocked

  _get_key: (e) ->
    key = String.fromCharCode e.which
    if not e.shiftKey
      key = do key.toLowerCase
    key

  _onkeydown: (e) ->
    key = @_get_key e
    if not DialogManager.on_input key
      @_pressed[key] = true

  _onkeyup: (e) ->
    key = @_get_key e
    delete @_blocked[key]
    delete @_pressed[key]


class Map
  constructor: (@size) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = (do @_get_random_tile for i in [0...@size.x*@size.y])
    for x in [0...@size.x]
      @_set_tile (new Point x, 0), '#'
      @_set_tile (new Point x, @size.y - 1), '#'
    for y in [0...@size.y]
      @_set_tile (new Point 0, y), '#'
      @_set_tile (new Point @size.x - 1, y), '#'
    @_set_tile (new Point 1, 1), '.'

  get_random_free_square: ->
    result = new Point -1, -1
    while (@get_tile result) == '#'
      result.x = _.random (@size.x - 1)
      result.y = _.random (@size.y - 1)
    result

  get_starting_square: ->
    new Point 1, 1

  get_tile: (square) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      return @_tiles[square.x*@size.y + square.y]
    '#'

  _get_random_tile: ->
    if (do Math.random) < 0.2 then '#' else '.'

  _set_tile: (square, tile) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      @_tiles[square.x*@size.y + square.y] = tile


class PixiData
  sprite_index = -1

  constructor: (data) ->
    # data allows keys in ['frame', 'shadow', 'y_offset'].
    # data.shadow allows keys in ['image', 'x_offset', 'y_offset', 'z_offset'].
    _.fast_extend @, data

  update: (data) ->
    frame = data.frame or 'standing'
    @frame = "#{@_sprite.image}-#{frame}-#{@_sprite.direction}"
    @id = @_sprite.id
    @invulnerability_frames = @_sprite.invulnerability_frames
    @position = @_sprite.position
    @shadow = data.shadow
    @y_offset = data.y_offset or 0
    # TODO(skishore): Once we have better dialog management, drop this field.
    @label = DialogManager._current?.get_label @_sprite.id


class Sprite
  sprite_index = -1

  constructor: (@stage, @image, @_default_state, start) ->
    sprite_index += 1
    @direction = Direction.DOWN
    @health = 4
    @id = sprite_index
    @invulnerability_frames = 0
    @position = start.scale Constants.grid
    @square = start
    @_pixi_data = new PixiData _sprite: @
    do @set_state

  collides: (sprite, tolerance) ->
    if not do sprite._can_collide
      return false
    grid = Constants.grid
    tolerance = tolerance or new Point 0, 0
    (not (@position.x + grid - tolerance.x <= sprite.position.x or
          sprite.position.x + grid - tolerance.x <= @position.x)) and
    (not (@position.y + grid - tolerance.y <= sprite.position.y or
          sprite.position.y + grid - tolerance.y <= @position.y))

  get_free_direction: ->
    options = []
    for key, move of Constants.moves
      square = new Point @square.x + move[0], @square.y + move[1]
      if @_check_square square
        options.push key
    if options.length == 0
      options = _.keys Constants.moves
    Constants.moves[_.sample options]

  is_player: ->
    @ == @stage.player

  move: (vector) ->
    vector = @_check_squares vector
    if not do vector.zero
      @position = @position.add vector
      [@square, _] = do @_get_square_and_overlap
    vector

  set_state: (state) ->
    @state?.on_exit?()
    @state = state or new @_default_state
    @state.sprite = @
    @state.on_enter?()

  update: ->
    if @invulnerability_frames > 0
      @invulnerability_frames -= 1
    else if (do @is_player) and (do @_collides_with_any)
      @set_state new KnockbackState
    @_pixi_data.update do @state.update

  _can_collide: ->
    not (@state instanceof DeathState) and \
    not (@state instanceof JumpingState) and \
    not (@state instanceof KnockbackState)

  _collides_with_any: ->
    if not do @_can_collide
      return false
    tolerance = (new Point 4, 4).scale Constants.twips_per_pixel
    for sprite in @stage.sprites
      if sprite != @ and @collides sprite, tolerance
        return true
    false

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
           not @_check_square square.add offset
        collided = true
        if (Math.abs overlap.x) <= half_grid and offset.x*move.x <= 0
          shove = Math.min speed, (Math.abs overlap.x) - tolerance
          move.x = -offset.x*shove
      if collided
        if offset.y < 0
          move.y = -@_gmod @position.y + tolerance
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
                 not @_check_square square.add offset
      offset.y = if overlap.y > 0 then 1 else -1
      if not @_check_square new Point square.x + offset.x, square.y
        collided = true
      else if (overlap.y > 0 or -overlap.y > tolerance) and
              not @_check_square square.add offset
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
          move.x = -@_gmod @position.x + tolerance
        else
          move.x = @_gmod tolerance - @position.x
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
    square = @position.subtract overlap
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
  new PixiData frame: if animate then 'walking' else 'standing'

_switch_state = (sprite, new_state) ->
  sprite.set_state new_state
  do sprite.state.update


class AttackingState
  ATTACK_FRAMES = [1, 2, 3]
  ATTACK_FRAME_MAP = {
    up: [['ur', 13, -13], ['uu', -2, -15], ['ul', -13, -13]]
    right: [['ur', 13, -13], ['rr', 15, 2], ['dr', 13, 13]]
    down: [['dl', -13, 13], ['dd', 2, 15], ['dr', 13, 13]]
    left: [['ul', -13, -13], ['ll', -15, 2], ['dl', -13, 13]]
  }
  ATTACK_LENGTH = 18

  constructor: ->
    @_cur_frame = 0

  update: ->
    @_cur_frame += 1
    index = do @_get_index
    if index > 2
      return _switch_state @sprite, new WalkingState
    sword_frame = @_get_sword_frame index
    @_maybe_hit_enemies sword_frame
    new PixiData {
      frame: "attacking#{index}"
      shadow: sword_frame
    }

  _get_index: ->
    value = @_cur_frame - 1
    for i in [0...ATTACK_FRAMES.length]
      value -= ATTACK_FRAMES[i]
      if value < 0
        return i
    ATTACK_FRAMES.length

  _get_sword_frame: (index) ->
    data = ATTACK_FRAME_MAP[@sprite.direction][index]
    image: "sword-#{data[0]}"
    x_offset: Constants.twips_per_pixel*data[1]
    y_offset: Constants.twips_per_pixel*data[2]

  _maybe_hit_enemies: (sword_frame) ->
    enemies_hit = @_get_enemies_hit sword_frame
    for enemy in enemies_hit
      if DialogManager._current? and \
         not DialogManager._current.can_attack enemy.id
        return @sprite.stage.set_state new InvertState
    for enemy in enemies_hit
      enemy.direction = Direction.OPPOSITE[@sprite.direction]
      enemy.set_state new KnockbackState

  _get_enemies_hit: (sword_frame) ->
    # The sword tile extends further than the last pixel in the sword blade.
    # Compute an adjusted sword offset that takes the blade length into account.
    base_offset = new Point sword_frame.x_offset, sword_frame.y_offset
    unit_offset = new Point (@_round base_offset.x), (@_round base_offset.y)
    reach = unit_offset.scale_to Constants.twips_per_pixel*ATTACK_LENGTH
    offset = (base_offset.subtract unit_offset).add reach
    offset.x = Math.round offset.x
    offset.y = Math.round offset.y
    # Shift the sprite, check for collisions, and shift back.
    result = []
    @sprite.position.x += offset.x
    @sprite.position.y += offset.y
    for sprite in @sprite.stage.sprites
      if sprite != @sprite and @sprite.collides sprite
        result.push sprite
    @sprite.position.x -= offset.x
    @sprite.position.y -= offset.y
    result

  _round: (value) ->
    Constants.grid*(Math.round value/Constants.grid)


class DeathState
  DEATH_FRAMES = 4

  on_enter: ->
    @_cur_frame = 0
    @sprite.direction = ''
    @sprite.image = 'explosion'

  update: ->
    @_cur_frame += 1
    index = Math.floor (@_cur_frame - 1)/DEATH_FRAMES
    if index > 1
      return @sprite.stage.destruct @sprite
    new PixiData frame: "red#{index}"


class JumpingState
  JUMP_HEIGHT = 1.0*Constants.grid
  JUMP_LENGTH = 3.0*Constants.grid
  JUMP_SPEED = 1.2*Constants.player_speed

  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil JUMP_LENGTH/JUMP_SPEED

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return _switch_state @sprite, new WalkingState
    keys = do @sprite.stage.input.get_keys_pressed
    _move_sprite.call @, _get_move keys, JUMP_SPEED
    new PixiData {
      frame: "jumping#{Math.floor 3*(@_cur_frame - 1)/@_max_frame}"
      shadow: {image: 'shadow'}
      y_offset: do @_get_y_offset
    }

  _get_y_offset: ->
    arc_position = (Math.pow 2*@_cur_frame/@_max_frame - 1, 2) - 1
    Math.floor JUMP_HEIGHT*arc_position


class KnockbackState
  KNOCKBACK_LENGTH = 1.5*Constants.grid
  KNOCKBACK_SPEED = 3*Constants.player_speed

  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil KNOCKBACK_LENGTH/KNOCKBACK_SPEED

  on_enter: ->
    @sprite.health -= 1
    @sprite.invulnerability_frames = \
        @_max_frame + Constants.extra_invulnerability_frames

  on_exit: ->
    if not do @sprite.is_player
      @sprite.invulnerability_frames = 0

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return _switch_state @sprite, if @sprite.health <= 0 then new DeathState
    [x, y] = Direction.UNIT_VECTOR[@sprite.direction]
    result = _move_sprite.call @, (new Point x, y).scale_to -KNOCKBACK_SPEED
    # Calling _move_sprite will flip the sprite direction, so we flip it back.
    @sprite.direction = Direction.OPPOSITE[@sprite.direction]
    result


class PausedState
  constructor: (random) ->
    speed = Constants.enemy_speed
    base_steps = Math.floor Constants.grid/speed
    @_steps = if random then (_.random -base_steps, base_steps) else base_steps

  update: ->
    @_steps -= 1
    if @_steps < 0
      return _switch_state @sprite, new RandomWalkState
    new PixiData frame: 'standing'


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
      return _switch_state @sprite, new PausedState true
    _move_sprite.call @, @_move


class WalkingState
  constructor: ->
    @_anim_num = 0
    @_period = Constants.walking_animation_frames

  update: ->
    keys = do @sprite.stage.input.get_keys_pressed
    if @_consume_input keys, 'j'
      return _switch_state @sprite, new JumpingState
    else if @_consume_input keys, 'k'
      return _switch_state @sprite, new AttackingState
    _move_sprite.call @, _get_move keys, Constants.player_speed

  _consume_input: (keys, key) ->
    if keys[key]?
      @sprite.stage.input.block_key key
      return true


class Stage
  constructor: ->
    # Construct the dialog so we know how many enemies we need.
    dialog = new (if (do Math.random) < 0.5 then TransliterationMatchingGame \
                  else EnglishToHindiMultipleChoiceGame)
    num = do dialog.get_num_enemies
    DialogManager.set_page dialog
    # Initialize the normal game state.
    @input = new Input
    @map = new Map new Point 19, 11
    @player = do @_construct_player
    @sprites = [@player].concat (do @_construct_enemy for i in [0...num])
    @set_state new GameplayState
    @_graphics = new Graphics @, $('.surface')
    @_sprites_to_destruct = []
    # Update the dialog with the enemy ids.
    for sprite in @sprites
      if sprite != @player
        DialogManager._current?.add_enemy sprite.id

  destruct: (sprite) ->
    @_sprites_to_destruct.push sprite
    DialogManager._current?.on_attack sprite.id
    # TODO(skishore): Advance to the next dialog here.

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  set_state: (state) ->
    @state = state
    @state.stage = @

  update: ->
    do @state.update
    for sprite in @_sprites_to_destruct
      @sprites = _.without @sprites, sprite
      if sprite == @player
        delete @player
    @_sprites_to_destruct.length = 0

  _construct_enemy: ->
    new Sprite @, 'enemy', PausedState, (do @map.get_random_free_square)

  _construct_player: ->
    new Sprite @, 'player', WalkingState, (do @map.get_starting_square)


class GameplayState
  update: ->
    for sprite in @stage.sprites
      do sprite.update
      # When a sprite update changes the game state, we should not continue
      # by updating the other sprites. Doing so could cause strange race
      # conditions (eg. the player scrolls the screen but is then killed).
      if @stage.state != @
        break


class InvertState
  constructor: ->
    @_frames_left = 2*Constants.invulnerability_animation_frames

  update: ->
    @_frames_left -= 1
    if @_frames_left < 0
      delete @stage._pixi_invert
      @stage.player.set_state new KnockbackState
      return @stage.set_state new GameplayState
    @stage._pixi_invert = true


#Meteor.startup (-> stage = new Stage) if Meteor.isClient
