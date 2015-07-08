class Graphics
  BORDER_IN_PIXELS = 2
  GRID_IN_PIXELS = 16

  constructor: (@stage, @element, callback) ->
    num_tiles = @stage.map.tileset.tiles.length
    size = do @stage.map.size.clone
    size.y += (Math.ceil num_tiles/size.x) + 1

    @scale = 2
    @size = size.scale @scale*GRID_IN_PIXELS
    @tiles = []
    @features = []

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y, {transparent: true}
    @element.prepend @renderer.view

    @context = new PIXI.Stage 0x00000000
    @tile_container = do @_add_container
    @feature_container = do @_add_container
    do @_initialize_stats

    assets_to_load = ['tileset']
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

  _make_tile: (square, image) ->
    tile = new PIXI.Sprite
    if image?
      tile.setTexture PIXI.Texture.fromFrame image
      tile._tilist_image = image
    tile.x = GRID_IN_PIXELS*square.x
    tile.y = GRID_IN_PIXELS*square.y
    tile

  _on_assets_loaded: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        square = new Point x, y
        tile = @_make_tile square, @stage.map.get_image square
        @tile_container.addChild tile
        @tiles.push tile
        feature = @_make_tile square
        @feature_container.addChild feature
        @features.push feature
    for choice, i in @stage.map.tileset.tiles
      @tile_container.addChild @_make_tile (@get_tileset_square i), choice.image
    do @stage.loop.bind @stage

  draw: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        image = @stage.map.get_image new Point x, y
        tile = @tiles[x*@stage.map.size.y + y]
        if tile._tilist_image != image
          tile.setTexture PIXI.Texture.fromFrame image
          tile._tilist_image = image
        image = @stage.map.get_feature_image new Point x, y
        feature = @features[x*@stage.map.size.y + y]
        if feature._tilist_image != image
          if image?
            feature.setTexture PIXI.Texture.fromFrame image
          else
            feature.setTexture PIXI.Texture.emptyTexture
          feature._tilist_image = image
    @renderer.render @context

  get_outline: (square) ->
    grid = @scale*GRID_IN_PIXELS
    outline = {
      left: "#{square.x*grid}px"
      top: "#{square.y*grid}px"
      font_size: "#{Math.floor 0.75*grid - 2*BORDER_IN_PIXELS}px"
      grid: "#{grid - 2*BORDER_IN_PIXELS}px"
    }

  get_target: (position) ->
    offset = do @element.offset
    position = do position.clone
    position.x -= offset.left
    position.y -= offset.top
    square = new Point \
        (Math.floor position.x/(@scale*GRID_IN_PIXELS)),
        (Math.floor position.y/(@scale*GRID_IN_PIXELS))
    outline = @get_outline square
    size = @stage.map.size
    if 0 <= square.x < size.x
      if 0 <= square.y < size.y
        return {
          type: 'tile'
          outline: outline
          square: square
        }
      index = (square.y - size.y - 1)*size.x + square.x
      if 0 <= index < @stage.map.tileset.tiles.length
        return {
          type: 'tileset'
          outline: outline
          index: index
        }
    type: 'none'

  get_tileset_square: (index) ->
    size = @stage.map.size
    new Point (index % size.x), (size.y + (Math.floor index/size.x) + 1)


class Input
  constructor: ->
    window.onkeydown = @_onkeydown.bind @
    window.onkeyup = @_onkeyup.bind @
    window.onmousemove = @_onmousemove.bind @
    @_blocked = {}
    @_pressed = {}
    @_mouse_position = new Point 0, 0

  block_key: (key) ->
    @_blocked[key] = true

  get_keys_pressed: ->
    _.fast_omit @_pressed, @_blocked

  get_mouse_position: ->
    @_mouse_position

  _get_key: (e) ->
    key = String.fromCharCode e.which
    if not e.shiftKey
      key = do key.toLowerCase
    key

  _onkeydown: (e) ->
    @_pressed[@_get_key e] = true

  _onkeyup: (e) ->
    key = @_get_key e
    delete @_blocked[key]
    delete @_pressed[key]

  _onmousemove: (e) ->
    @_mouse_position.x = e.x
    @_mouse_position.y = e.y


class Map
  EDGES = [['u', 0, -1], ['d', 0, 1], ['r', 1, 0], ['l', -1, 0]]

  constructor: (@size, @tileset) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = @_construct_2d_array @tileset.default_tile.index
    @_features = @_construct_2d_array undefined

  get_image: (square) ->
    assert @_in_bounds square
    tile = @tileset.tiles[@_tiles[square.x][square.y]]
    image = tile.image
    if tile.edging?
      for [direction, x, y] in EDGES
        other = @get_tile square.add new Point x, y
        if (not other.default) and (not tile.edging other)
          image += direction
    image

  get_feature_image: (square) ->
    assert @_in_bounds square
    feature = @_features[square.x][square.y]
    if feature? then @tileset.tiles[feature].image

  get_tile: (square) ->
    if @_in_bounds square
      return @tileset.tiles[@_tiles[square.x][square.y]]
    _.fast_extend {default: true}, @tileset.default_tile

  set_tile: (square, tile) ->
    assert @_in_bounds square
    if tile.feature
      base = @tileset.tiles[@_tiles[square.x][square.y]]
      if tile.constraint? and not tile.constraint base
        alternatives = @tileset.get_sorted_alternatives base
        @set_tile square, (_.filter alternatives, tile.constraint)[0]
      @_features[square.x][square.y] = tile.index
    else
      @_tiles[square.x][square.y] = tile.index
      @_features[square.x][square.y] = undefined
      @_fix_constraints square

  _construct_2d_array: (value) ->
    ((value for y in [0...@size.y]) for x in [0...@size.x])

  _fix_constraints: (square) ->
    tile = @get_tile square
    for [direction, x, y] in EDGES
      other_square = square.add new Point x, y
      other = @get_tile other_square
      if @tileset.violates_constraint tile, other
        for alternative in @tileset.get_sorted_alternatives other
          if not @tileset.violates_constraint tile, alternative
            @set_tile other_square, alternative
            break

  _in_bounds: (square) ->
    0 <= square.x < @size.x and 0 <= square.y < @size.y


class Stage
  HOTKEYS = 'qwertyu12345678'

  constructor: ->
    @input = new Input
    @map = new Map (new Point 18, 11), new Tileset
    @_graphics = new Graphics @, $('.surface')
    @_num_hotkeys = Math.min @map.tileset.tiles.length, HOTKEYS.length
    assert @_num_hotkeys > 0
    @_hotkeys = {}
    for i in [0...@_num_hotkeys]
      @_hotkeys[i] = i
    do @_redraw_hotkeys

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    target = @_graphics.get_target do @input.get_mouse_position
    if target.type != 'tile'
      return Session.set 'tilist.cursor', null
    Session.set 'tilist.cursor', target.outline
    for key of do @input.get_keys_pressed
      index = HOTKEYS.indexOf key
      if index >= 0 and @_hotkeys[index]?
        @map.set_tile target.square, @map.tileset.tiles[@_hotkeys[index]]

  _redraw_hotkeys: ->
    hotkeys = []
    for i in [0...@_num_hotkeys]
      if not @_hotkeys[i]?
        continue
      square = @_graphics.get_tileset_square @_hotkeys[i]
      hotkeys.push @_graphics.get_outline square
      hotkeys[hotkeys.length - 1].text = do HOTKEYS[i].toUpperCase
    Session.set 'tilist.hotkeys', hotkeys

  _set_hotkey: (hotkey, value) ->
    if value < @map.tileset.tiles.length
      @_hotkeys[hotkey] = value
    else
      delete @_hotkeys[hotkey]


class Tileset
  constructor: ->
    can_border_water = (tile) -> tile.image in ['grass-yellow-', 'water']
    is_green = (tile) -> tile.image.startsWith 'grass-green'
    is_solid = (tile) -> tile.image != 'water'
    is_yellow = (tile) -> tile.image == 'grass-yellow-'
    # For a regular tile, a constraint is a predicate that must be true of the
    # four tiles adjacent by cardinal directions. For a feature, a constraint
    # is a predicate that must be true of the tile underneath it.
    @tiles = [
      {image: 'grass-green-', edging: is_green}
      {image: 'grass-green-flat', constraint: is_green}
      {image: 'grass-green-flower', constraint: is_green}
      {image: 'grass-green-tall', constraint: is_green}
      {image: 'grass-yellow-', edging: is_solid}
      {image: 'grass-yellow-flat', constraint: is_solid}
      {image: 'water', constraint: can_border_water}
    ]
    features = [
      {image: 'bush', constraint: is_yellow}
      {image: 'flower', constraint: is_yellow}
      {image: 'rock', constraint: is_yellow}
      {image: 'sign', constraint: is_yellow}
      {image: 'tree-ul', tree_offset: [0, 0], constraint: is_yellow}
      {image: 'tree-ur', tree_offset: [1, 0], constraint: is_yellow}
      {image: 'tree-dl', tree_offset: [0, 1], constraint: is_yellow}
      {image: 'tree-dr', tree_offset: [1, 1], constraint: is_yellow}
    ]
    for feature in features
      feature.feature = true
      @tiles.push feature
    tiles_by_image = {}
    for tile, i in @tiles
      tile.index = i
      tiles_by_image[tile.image] = tile
    @default_tile = tiles_by_image['grass-green-']
    # A constraint is valid if at least one of the fallback tiles satisfies it.
    @_fallback_tiles = [@default_tile, tiles_by_image['grass-yellow-']]
    do @_check_constraint_validity

  get_sorted_alternatives: (tile) ->
    good_alternatives = []
    bad_alternatives = []
    for alternative in @_fallback_tiles
      if @violates_constraint tile, alternative
        bad_alternatives.push alternative
      else
        good_alternatives.push alternative
    good_alternatives.concat bad_alternatives

  violates_constraint: (tile1, tile2) ->
    if tile1.default or tile2.default
      return false
    (tile1.constraint? and not tile1.constraint tile2) or \
    (tile2.constraint? and not tile2.constraint tile1)

  _check_constraint_validity: ->
    for tile in @tiles
      assert (not tile.constraint?) or \
             _.any (tile.constraint fallback for fallback in @_fallback_tiles)


if Meteor.isClient
  Template.tilist.helpers {
    outlines: ->
      result = Session.get 'tilist.hotkeys'
      cursor = Session.get 'tilist.cursor'
      if cursor?
        result.push cursor
      result
  }
  Meteor.startup (-> stage = new Stage)
