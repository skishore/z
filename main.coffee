TWIPS_PER_PIXEL = 1024
GRID = base.grid_in_pixels*TWIPS_PER_PIXEL
MOVEMENT_KEYS = {w: [0, -1], a: [-1, 0], s: [0, 1], d: [1, 0]}

# Tolerance is the distance a sprite can move into a blocked square.
HALF_GRID = Math.ceil 0.5*GRID
TOLERANCE = Math.ceil 0.2*GRID

BASE_SPEED = 0.08*GRID
WALKING_ANIMATION_FRAMES = 12
INVULNERABILITY_ANIMATION_FRAMES = 6
EXTRA_INVULNERABILITY_FRAMES = 15


class Direction
  @UP = 'up'
  @RIGHT = 'right'
  @DOWN = 'down'
  @LEFT = 'left'

  @OPPOSITE = {up: 'down', right: 'left', down: 'up', left: 'right'}
  @UNIT_VECTOR = {up: [0, -1], right: [1, 0], down: [0, 1], left: [-1, 0]}
  @OPTIONS = _.keys @UNIT_VECTOR

  @get_move_direction: (move, last_direction) ->
    options = []
    options.push Direction.UP if move.y < 0
    options.push Direction.RIGHT if move.x > 0
    options.push Direction.DOWN if move.y > 0
    options.push Direction.LEFT if move.x < 0
    if options.length > 0 and (options.indexOf last_direction) < 0
      return options[0]
    last_direction


class Graphics extends base.Graphics
  constructor: (@stage, @element) ->
    super @stage, @element
    @sprite_container = @_add_container @layers.game
    @sprites = {}

  draw: ->
    drawn = {}
    for sprite in @stage.sprites
      @_draw_sprite sprite._pixi_data, drawn
    ids_to_remove = (id for id of @sprites when not drawn[id])
    for id in ids_to_remove
      @_remove_sprite id
    @sprite_container.children.sort (a, b) -> Math.sign b.z - a.z
    @layers.game.filters = if @stage._pixi_invert \
                           then [new PIXI.filters.InvertFilter] else null
    super

  prepare_scroll: (speed, offset) ->
    @sprites[@stage.player?._pixi_data.id]?.visible = false
    super speed, offset
    @sprites[@stage.player?._pixi_data.id]?.visible = true

  _draw_shadow: (sprite, drawn) ->
    if not sprite.shadow?
      return
    shadow = sprite.shadow
    shadow_id = "#{sprite.id}shadow"
    pixi = @_get_sprite shadow_id
    pixi.x = @_to_pixels sprite.position.x + (shadow.x_offset or 0)
    pixi.y = @_to_pixels sprite.position.y + (shadow.y_offset or 0)
    pixi.z = -sprite.position.y + (shadow.z_offset or 0)
    pixi.texture = PIXI.utils.TextureCache[shadow.image] or PIXI.Texture.EMPTY
    drawn[shadow_id] = true

  _draw_sprite: (sprite, drawn) ->
    if not sprite.id?
      return
    pixi = @_get_sprite sprite.id
    pixi.x = @_to_pixels sprite.position.x
    pixi.y = @_to_pixels sprite.position.y + sprite.y_offset
    pixi.z = -sprite.position.y + GRID*sprite.y_offset
    pixi.texture = PIXI.utils.TextureCache[sprite.frame] or PIXI.Texture.EMPTY
    if sprite.invulnerability_frames == 0
      pixi.filters = null
    else
      period = INVULNERABILITY_ANIMATION_FRAMES
      pixi.filters = if sprite.invulnerability_frames % (2*period) < period \
                     then [new PIXI.filters.InvertFilter] else null
    drawn[sprite.id] = true
    @_draw_shadow sprite, drawn
    @_draw_text sprite

  _draw_text: (sprite) ->
    if not sprite.label?
      return
    element = @_get_text sprite.id, sprite.label
    element?.css 'transform', "translateX(-50%) translate(#{
      @scale*@_to_pixels sprite.position.x + GRID/2}px, #{
      @scale*((@_to_pixels sprite.position.y + GRID) + 1)}px)"

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

  _to_pixels: (twips) ->
    Math.round twips/TWIPS_PER_PIXEL


class Map extends base.Map
  PERIOD = 720
  TILESET = {
    bush: {blocked: true, cuttable: true, particles: {num: 4, images: ['leaf']}}
    flower: {
      blocked: true
      cuttable: true
      particles: {num: 4, images: ['leaf', 'petal']}
    }
    water: {animation: {frames: 4, period: 24}, blocked: true, water: true}
    default: {blocked: true}
    free: {}
  }

  constructor: (@stage, uid) ->
    super uid
    @_frame = 0
    @starting_square = new Point (Math.floor @size.x/2), 0
    if @stage.player? and @_in_bounds @stage.player.square
      @starting_square = @stage.player.square
    @starting_direction = @_get_edge_direction @starting_square
    do @_clear_sprites
    do @_maybe_lock_doors
    do @_maybe_spawn_enemies

  get_map_data: (square) ->
    if not @_in_bounds square
      return {blocked: true, out_of_bounds: true}
    feature = @_features[square.x][square.y]
    if feature?
      data = TILESET[feature]
      return if data? then data else TILESET.default
    data = TILESET[@_tiles[square.x][square.y]]
    return if data? then data else TILESET.free

  get_tile_image: (square) ->
    result = super square
    animation = TILESET[result]?.animation
    if animation?
      period = animation.frames*animation.period
      index = Math.floor (@_frame % period)/animation.period
      return "#{result}-#{index}"
    if result[result.length - 1] == '-' and @_features[square.x][square.y]?
      result += 'flat'
    result

  on_attack: (square) ->
    map_data = @get_map_data square
    if map_data.cuttable
      @_features[square.x][square.y] = undefined
      for i in [0...(map_data.particles?.num or 0)]
        image = _.sample map_data.particles.images
        data = {image: image, fixed: {default_state: ParticleState}}
        @stage.sprites.push new Sprite @stage, data, square

  update: ->
    @_frame = (@_frame + 1) % PERIOD
    do @_maybe_unlock_doors

  _apply_to_edges: (callback) ->
    for x in [0...@size.x]
      callback new Point x, 0
      callback new Point x, @size.y - 1
    for y in [0...@size.y]
      callback new Point 0, y
      callback new Point @size.x - 1, y

  _clear_sprites: ->
    @stage.sprites = if @stage.player? then [@stage.player] else []

  _get_edge_direction: (square) ->
    if square.x == 0
      return Direction.RIGHT
    else if square.x == @size.x - 1
      return Direction.LEFT
    else if square.y == @size.y - 1
      return Direction.UP
    Direction.DOWN

  _get_free_square: ->
    result = new Point -1, -1
    while (@get_map_data result).blocked
      result.x = _.random (@size.x - 1)
      result.y = _.random (@size.y - 1)
    result

  _maybe_lock_doors: ->
    # TODO(skishore): We should read map data to check if the room should
    # be locked, and then check if there are enemies to lock it.
    @_apply_to_edges (square) =>
      if (@get_map_data square).blocked
        return
      direction = @_get_edge_direction square
      @_features[square.x][square.y] = "door-#{direction}"
      if square.equals @starting_square
        [x, y] = Direction.UNIT_VECTOR[direction]
        @starting_square = @starting_square.add new Point x, y
    @_locked = true

  _maybe_spawn_enemies: ->
    for i in [0..._.random 8]
      @stage.spawn (_.sample ['moblin', 'zol']), do @_get_free_square

  _maybe_unlock_doors: ->
    if not @_locked or do @_should_lock_doors
      return
    @_apply_to_edges (square) =>
      if @_features[square.x][square.y].startsWith 'door-'
        delete @_features[square.x][square.y]
    delete @_locked

  _should_lock_doors: ->
    # TODO(skishore): Even if there are non-player sprites on the screen,
    # it may be okay to unlock the doors - for example, leaves are sprites
    # but they should not prevent a player from leaving.
     _.any @stage.sprites, (sprite) -> not do sprite.is_player


class PixiData
  sprite_index = -1

  constructor: (data) ->
    # data allows keys in ['frame', 'shadow', 'y_offset'].
    # data.shadow allows keys in ['image', 'x_offset', 'y_offset', 'z_offset'].
    _.fast_extend @, data

  update: (data) ->
    frame = data.frame or 'standing'
    direction = if @_sprite.fixed.unoriented then '' else @_sprite.direction
    @frame = "#{@_sprite.image}-#{frame}-#{direction}"
    @id = @_sprite.id
    @invulnerability_frames = @_sprite.invulnerability_frames
    @position = @_sprite.position
    @shadow = data.shadow
    @y_offset = data.y_offset or 0
    # TODO(skishore): Once we have better dialog management, drop this field.
    @label = DialogManager._current?.get_label @_sprite.id


class Sprite
  sprite_index = -1

  constructor: (@stage, data, start) ->
    sprite_index += 1
    @id = sprite_index
    _.fast_extend @, data
    @invulnerability_frames = 0
    @reposition start
    do @set_state
    @_pixi_data = new PixiData _sprite: @
    @_pixi_data.update {}

  collides: (sprite, tolerance) ->
    if not sprite.state.collides
      return false
    tolerance = tolerance or new Point 0, 0
    (not (@position.x + GRID - tolerance.x <= sprite.position.x or
          sprite.position.x + GRID - tolerance.x <= @position.x)) and
    (not (@position.y + GRID - tolerance.y <= sprite.position.y or
          sprite.position.y + GRID - tolerance.y <= @position.y))

  get_free_direction: ->
    options = []
    for key, move of MOVEMENT_KEYS
      square = new Point @square.x + move[0], @square.y + move[1]
      if @_check_square square
        options.push key
    if options.length == 0
      options = _.keys MOVEMENT_KEYS
    MOVEMENT_KEYS[_.sample options]

  is_player: ->
    @fixed.default_state == WalkingState

  move: (vector, skip_checks) ->
    if not skip_checks
      vector = @_check_squares vector
    if not do vector.zero
      @_set_position @position.add vector
    vector

  reposition: (square) ->
    @_set_position square.scale GRID
    if do @is_player
      @direction = @stage.map.starting_direction
    else
      @direction = _.sample Direction.OPTIONS

  set_state: (state) ->
    @state?.on_exit?()
    @state = state or new @fixed.default_state
    @state.sprite = @
    @state.on_enter?()

  switch_state: (state) ->
    @set_state state
    do @_check_static_conditions
    do @state.update

  update: ->
    square = do @square.clone
    if @invulnerability_frames > 0
      @invulnerability_frames -= 1
    do @_check_static_conditions
    @_pixi_data.update do @state.update

  _collides_with_any: ->
    tolerance = new Point 0.25*GRID, 0.25*GRID
    for sprite in @stage.sprites
      if sprite != @ and @collides sprite, tolerance
        return true
    false

  _check_squares: (move) ->
    move = new Point (Math.round move.x), (Math.round move.y)
    if do move.zero
      return move

    # Sprites may not move faster than our collision detection can handle.
    # If this restriction becomes a problem, we can get around it by breaking
    # the move up into several steps.
    speed = Math.floor do move.length
    assert speed < HALF_GRID
    offset = new Point 0, 0
    collided = false

    # Check if we cross a horizontal grid boundary going up or down.
    if move.y < 0 and (@_gmod @position.y + TOLERANCE) < -move.y
      offset.y = -1
    else if move.y > 0 and (@_gmod -@position.y) < move.y
      offset.y = 1
    # If we cross a horizontal boundary, check that the next square is open.
    if offset.y != 0
      offset.x = if @overlap.x > 0 then 1 else -1
      if not @_check_square new Point @square.x, @square.y + offset.y
        collided = true
      else if (Math.abs @overlap.x) > TOLERANCE and \
           not @_check_square @square.add offset
        collided = true
        if (Math.abs @overlap.x) <= HALF_GRID and offset.x*move.x <= 0
          shove = Math.min speed, (Math.abs @overlap.x) - TOLERANCE
          move.x = -offset.x*shove
      if collided
        if offset.y < 0
          move.y = -@_gmod @position.y + TOLERANCE
        else
          move.y = @_gmod -@position.y
    # Run similar checks for crossing a vertical boundary going right or left.
    offset.x = 0
    if move.x < 0 and (@_gmod @position.x + TOLERANCE) < -move.x
      offset.x = -1
    else if move.x > 0 and (@_gmod TOLERANCE - @position.x) < move.x
      offset.x = 1
    if offset.x != 0
      # If we've crossed a horizontal boundary (which is true iff offset.y != 0
      # and collided is false) then run an extra check in the x direction.
      collided = offset.y != 0 and not collided and \
                 not @_check_square @square.add offset
      offset.y = if @overlap.y > 0 then 1 else -1
      if not @_check_square new Point @square.x + offset.x, @square.y
        collided = true
      else if (@overlap.y > 0 or -@overlap.y > TOLERANCE) and
              not @_check_square @square.add offset
        collided = true
        if (Math.abs @overlap.y) <= HALF_GRID and offset.y*move.y <= 0
          # Check that we have space to shove away in the y direction.
          # We skip this check when shoving in the x direction because the
          # full x check is after the y check.
          shove = Math.min speed, Math.max @overlap.y, -@overlap.y - TOLERANCE
          square = new Point @square.x, @square.y + offset.y
          move.y = if @_check_square square then -offset.y*shove else 0
      if collided
        if offset.x < 0
          move.x = -@_gmod @position.x + TOLERANCE
        else
          move.x = @_gmod TOLERANCE - @position.x
    move

  _check_square: (square) ->
    map_data = @stage.map.get_map_data square
    can_move_on_water = ((do @is_player) or @state instanceof KnockbackState)
    (not map_data.blocked) or (can_move_on_water and map_data.water) or \
    ((do @is_player) and map_data.out_of_bounds)

  _check_static_conditions: ->
    # Check if the sprite can be hurt by contact, and if so, check if they do.
    if @state.collides and (do @is_player) and \
       @invulnerability_frames == 0 and (do @_collides_with_any)
      @set_state new KnockbackState
    # Check if the sprite can drown, and if so, check if they are on water.
    if @state.collides or @state instanceof KnockbackState
      if (@stage.map.get_map_data @square).water
        @set_state new DrowningState
      else if @overlap.y > 0
        one_square_down = new Point @square.x, @square.y + 1
        if (@stage.map.get_map_data one_square_down).water
          @set_state new DrowningState

  _gmod: (value) ->
    result = value % GRID
    if result >= 0 then result else result + GRID

  _set_position: (@position) ->
    # We need to update _pixi_data, because a sprite can be moved outside of
    # a call to @state.update (for example, when the player scrolls the screen).
    @_pixi_data?.position = @position
    # overlap is a point (x, y) such x in [-HALF_GRID, HALF_GRID) and such that
    # position.x == overlap.x mod GRID, and similarly for y.
    @overlap = new Point (@_gmod @position.x), (@_gmod @position.y)
    @overlap.x -= if @overlap.x < HALF_GRID then 0 else GRID
    @overlap.y -= if @overlap.y < HALF_GRID then 0 else GRID
    # By subtracting overlap from position we round it to a grid square.
    @square = @position.subtract @overlap
    assert (@square.x % GRID == 0) and (@square.y % GRID == 0)
    @square.x /= GRID
    @square.y /= GRID


_get_move = (keys, speed) ->
  move = new Point 0, 0
  for key of keys
    if key of MOVEMENT_KEYS
      [x, y] = MOVEMENT_KEYS[key]
      move.x += x
      move.y += y
  if (do move.zero) then move else move.scale_to speed

_move_sprite = (attempt) ->
  move = @sprite.move attempt
  if not do move.zero and @_period?
    animation = @sprite.fixed.animation or ['standing', 'walking']
    period = Math.round @_period*BASE_SPEED/((do move.length)*animation.length)
    period = Math.max period, 1
    index = Math.floor (@_anim_num % (animation.length*period))/period
    frame = animation[index]
    @_anim_num += 1
  @sprite.direction = Direction.get_move_direction attempt, @sprite.direction
  new PixiData frame: frame


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
    @collides = true

  update: ->
    @_cur_frame += 1
    index = do @_get_index
    if index > 2
      return @sprite.switch_state new WalkingState
    sword_frame = @_get_sword_frame index
    @_maybe_hit_enemies sword_frame
    @_maybe_hit_features sword_frame
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
    x_offset: TWIPS_PER_PIXEL*data[1]
    y_offset: TWIPS_PER_PIXEL*data[2]

  _maybe_hit_enemies: (sword_frame) ->
    enemies_hit = @_get_enemies_hit sword_frame
    for enemy in enemies_hit
      if DialogManager._current? and \
         not DialogManager._current.can_attack enemy.id
        return @sprite.stage.set_state new InvertState
    for enemy in enemies_hit
      enemy.direction = Direction.OPPOSITE[@sprite.direction]
      enemy.set_state new KnockbackState

  _maybe_hit_features: (sword_frame) ->
    diff = new Point (@_round sword_frame.x_offset), \
                     (@_round sword_frame.y_offset)
    diff.x /= GRID
    diff.y /= GRID
    @sprite.stage.map.on_attack @sprite.square.add diff

  _get_enemies_hit: (sword_frame) ->
    # The sword tile extends further than the last pixel in the sword blade.
    # Compute an adjusted sword offset that takes the blade length into account.
    base_offset = new Point sword_frame.x_offset, sword_frame.y_offset
    unit_offset = new Point (@_round base_offset.x), (@_round base_offset.y)
    reach = unit_offset.scale_to TWIPS_PER_PIXEL*ATTACK_LENGTH
    offset = (base_offset.subtract unit_offset).add reach
    offset.x = @_round offset.x
    offset.y = @_round offset.y
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
    GRID*Math.round value/GRID


class DeathState
  DEATH_FRAMES = 4

  on_enter: ->
    @_cur_frame = 0
    @sprite.direction = ''
    @sprite.image = 'explosion'
    @sprite.invulnerability_frames = 0

  update: ->
    @_cur_frame += 1
    index = Math.floor (@_cur_frame - 1)/DEATH_FRAMES
    if index > 1
      @sprite.stage.destruct @sprite
    new PixiData frame: "red#{index}"


class DrowningState
  DROWNING_FRAMES = 12

  on_enter: ->
    @_cur_frame = 0
    @sprite.direction = ''
    @sprite.health -= 1
    [@_image, @sprite.image] = [@sprite.image, 'drowning']

  on_exit: ->
    @sprite.image = @_image
    @sprite.invulnerability_frames = EXTRA_INVULNERABILITY_FRAMES
    @sprite.reposition @sprite.stage.map.starting_square

  update: ->
    @_cur_frame += 1
    index = Math.floor (@_cur_frame - 1)/DROWNING_FRAMES
    if index > 1
      if do @sprite.is_player
        return @sprite.switch_state if @sprite.health <= 0 then new DeathState
      @sprite.stage.destruct @sprite
    new PixiData frame: "#{index}"


class JumpingState
  JUMP_HEIGHT = 1.0*GRID
  JUMP_LENGTH = 3.2*GRID
  JUMP_SPEED = 1.2*BASE_SPEED

  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil JUMP_LENGTH/JUMP_SPEED

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return @sprite.switch_state new WalkingState
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
  KNOCKBACK_LENGTH = 1.5*GRID
  KNOCKBACK_SPEED = 3*BASE_SPEED

  constructor: ->
    @_cur_frame = 0
    @_max_frame = Math.ceil KNOCKBACK_LENGTH/KNOCKBACK_SPEED

  on_enter: ->
    @sprite.health -= 1
    @sprite.invulnerability_frames = @_max_frame
    if do @sprite.is_player
      @sprite.invulnerability_frames += EXTRA_INVULNERABILITY_FRAMES
    @_speed = KNOCKBACK_SPEED/(@sprite.fixed.weight or 1)

  update: ->
    @_cur_frame += 1
    if @_cur_frame >= @_max_frame
      return @sprite.switch_state if @sprite.health <= 0 then new DeathState
    [x, y] = Direction.UNIT_VECTOR[@sprite.direction]
    result = _move_sprite.call @, (new Point x, y).scale_to -@_speed
    # Calling _move_sprite will flip the sprite direction, so we flip it back.
    @sprite.direction = Direction.OPPOSITE[@sprite.direction]
    result


class ParticleState
  DISTANCE = 0.5*GRID
  MIN_SPEED = 0.25*BASE_SPEED
  SIZE = [8, 6]

  constructor: ->
    while (not @velocity?) or (do @velocity.length) < MIN_SPEED
      @velocity = (new Point (do @_normal), (do @_normal)).scale 2*MIN_SPEED
    v = @velocity
    @frame = "#{if v.y < 0 then 'u' else 'd'}#{if v.x < 0 then 'l' else 'r'}"
    @_frames_left = DISTANCE/(do @velocity.length)

  on_enter: ->
    [x, y] = SIZE
    @sprite.direction = ''
    @sprite.position.x += _.random TWIPS_PER_PIXEL*x
    @sprite.position.y += _.random TWIPS_PER_PIXEL*x

  update: ->
    @_frames_left -= 1
    if @_frames_left < 0
      @sprite.stage.destruct @sprite
    @sprite.position = @sprite.position.add @velocity
    new PixiData frame: @frame

  _normal: ->
    u = 2*(do Math.random) - 1
    v = 2*(do Math.random) - 1
    r = u*u + v*v
    if 0 < r <= 1
      return u*Math.sqrt -2*(Math.log r)/r
    do @_normal


class PausedState
  constructor: (@random) ->
    @collides = true

  on_enter: ->
    base_steps = Math.floor GRID/(@sprite.fixed.speed*BASE_SPEED)
    @_steps = if @random then (_.random -base_steps, base_steps) else base_steps

  update: ->
    @_steps -= 1
    if @_steps < 0
      return @sprite.switch_state new RandomWalkState
    new PixiData frame: 'standing'


class RandomWalkState
  constructor: ->
    @_anim_num = 0
    @_period = WALKING_ANIMATION_FRAMES
    @collides = true

  on_enter: ->
    speed = @sprite.fixed.speed*BASE_SPEED
    base_steps = Math.floor GRID/speed
    [x, y] = do @sprite.get_free_direction
    @_move = (new Point x, y).scale_to speed
    @_steps = _.random 1, 4*base_steps

  update: ->
    @_steps -= 1
    if @_steps < 0
      return @sprite.switch_state new PausedState true
    _move_sprite.call @, @_move


class WalkingState
  constructor: ->
    @_anim_num = 0
    @_period = WALKING_ANIMATION_FRAMES
    @collides = true

  update: ->
    keys = do @sprite.stage.input.get_keys_pressed
    if @_consume_input keys, 'j'
      return @sprite.switch_state new JumpingState
    else if @_consume_input keys, 'k'
      return @sprite.switch_state new AttackingState
    _move_sprite.call @, _get_move keys, @sprite.fixed.speed*BASE_SPEED

  _consume_input: (keys, key) ->
    if keys[key]?
      @sprite.stage.input.block_key key
      return true


class Stage
  MONSTERS = {
    player: {
      image: 'player'
      health: 4
      fixed: {default_state: WalkingState, speed: 1}
    }
    moblin: {
      image: 'moblin'
      health: 2
      fixed: {default_state: PausedState, speed: 0.5}
    }
    zol: {
      image: 'zol'
      health: 2
      fixed: {
        animation: ['standing', 'midheight', 'walking',
                    'walking', 'midheight', 'standing']
        default_state: PausedState
        speed: 0.25
        unoriented: true
        weight: 2
      }
    }
  }
  SCROLL_SPEED = 16

  constructor: ->
    @input = new base.Input {keyboard: true}
    @map = new Map @, base.starting_map_uid
    @player = @spawn 'player', @map.starting_square
    @set_state new GameplayState
    @_graphics = new Graphics @, $('.surface')
    @_ids_to_remove = {}

  destruct: (sprite) ->
    @_ids_to_remove[sprite.id] = true
    DialogManager._current?.on_attack sprite.id
    # TODO(skishore): Maybe advance to the next dialog here.

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  maybe_scroll: ->
    if not (@player? and (@map.get_map_data @player.square).out_of_bounds)
      return
    # Compute the screen offset and shift the player to the new screen.
    offset = new Point (@_sign @player.square.x, @map.size.x), \
                       (@_sign @player.square.y, @map.size.y)
    move = (new Point offset.x*@map.size.x, offset.y*@map.size.y).scale -GRID
    @player.move move, true # skip_checks
    # Load the new map and scroll to the new screen.
    @_graphics.prepare_scroll SCROLL_SPEED, offset
    @map = new Map @, @map.get_uid offset
    @set_state new ScrollState @_graphics

  set_state: (state) ->
    @state = state
    @state.stage = @

  spawn: (breed, square) ->
    sprite = new Sprite @, MONSTERS[breed], square
    @sprites.push sprite
    sprite

  update: ->
    do @state.update
    @sprites = @sprites.filter (sprite) => not @_ids_to_remove[sprite.id]
    delete @player if @_ids_to_remove[@player?.id]
    @_ids_to_remove = {}

  _sign: (value, max) ->
    if value < 0 then -1 else if value >= max then 1 else 0


class GameplayState
  update: ->
    do @stage.map.update
    for sprite in @stage.sprites
      do sprite.update
      # When a sprite update changes the game state, we should not continue
      # by updating the other sprites. Doing so could cause strange race
      # conditions (eg. the player scrolls the screen but is then killed).
      if @stage.state != @
        return
    do @stage.maybe_scroll


class InvertState
  constructor: ->
    @_frames_left = 2*INVULNERABILITY_ANIMATION_FRAMES

  update: ->
    @_frames_left -= 1
    if @_frames_left < 0
      delete @stage._pixi_invert
      @stage.player.set_state new KnockbackState
      return @stage.set_state new GameplayState
    @stage._pixi_invert = true


class ScrollState
  constructor: (@graphics) ->

  update: ->
    if do @graphics.scroll
      @stage.set_state new GameplayState


base.modes.main = Stage
