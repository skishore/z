class Graphics extends base.Graphics
  BORDER_IN_PIXELS = 2

  constructor: (@stage, callback) ->
    num_tiles = @stage.tileset.tiles.length
    size = do @stage.map.size.clone
    size.y += (Math.ceil num_tiles/size.x) + 1
    super @stage, {assets: ['tileset'], size: size, transparent: true}
    @tileset_container = @_add_container @layers.ui

  _on_assets_loaded: ->
    for choice, i in @stage.tileset.tiles
      @_make_tile (@get_tileset_square i), choice.image, @tileset_container
    super

  get_outline: (square) ->
    grid = @scale*base.grid_in_pixels
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
        (Math.floor position.x/(@scale*base.grid_in_pixels)),
        (Math.floor position.y/(@scale*base.grid_in_pixels))
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
      if 0 <= index < @stage.tileset.tiles.length
        return {
          type: 'tileset'
          outline: outline
          index: index
        }
    type: 'none'

  get_tileset_square: (index) ->
    size = @stage.map.size
    new Point (index % size.x), (size.y + (Math.floor index/size.x) + 1)


class Map extends base.Map
  EDGES = [['u', 0, -1], ['d', 0, 1], ['r', 1, 0], ['l', -1, 0]]

  constructor: (@tileset, @uid) ->
    @size = new Point base.map_size[0], base.map_size[1]
    @_tiles = @_construct_2d_array @tileset.default_tile.index
    @_features = @_construct_2d_array []
    @load uid, {raw: true}

  clear_features: (square) ->
    if @_features[square.x][square.y].length > 0
      @_features[square.x][square.y].length = 0
      return true
    false

  get_feature_image: (square) ->
    assert @_in_bounds square
    images = (@tileset.tiles[i].image for i in @_features[square.x][square.y])
    if images.length > 0 then (do images.sort).join ' '

  get_tile: (square) ->
    if @_in_bounds square
      return @tileset.tiles[@_tiles[square.x][square.y]]
    _.fast_extend {default: true}, @tileset.default_tile

  get_tile_image: (square) ->
    assert @_in_bounds square
    tile = @tileset.tiles[@_tiles[square.x][square.y]]
    image = tile.image
    if tile.edging?
      for [direction, x, y] in EDGES
        other = @get_tile square.add new Point x, y
        if (not other.default) and (not tile.edging other)
          image += direction
    image

  set_tile: (square, tile) ->
    # Returns true if a tile changed.
    if not @_in_bounds square
      return false
    if tile.feature
      if tile.index in @_features[square.x][square.y]
        return false
      base = @tileset.tiles[@_tiles[square.x][square.y]]
      if tile.constraint? and not tile.constraint base
        alternatives = @tileset.get_sorted_alternatives base
        # If we have to modify the tile below, we try to preserve any existing
        # features on it in case they are compatible with the new feature.
        features = _.clone @_features[square.x][square.y]
        @set_tile square, (_.filter alternatives, tile.constraint)[0]
        @_features[square.x][square.y] = features
      @_set_feature square, tile
    else
      if tile.index == @_tiles[square.x][square.y] and \
         @_features[square.x][square.y].length == 0
        return false
      @_tiles[square.x][square.y] = tile.index
      @_features[square.x][square.y].length = 0
      @_fix_constraints square
    true

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

  _set_feature: (square, feature) ->
    assert @_in_bounds square
    assert feature.feature
    if @_features[square.x][square.y].length == 0
      @_features[square.x][square.y] = [feature.index]
      return
    if feature.index in @_features[square.x][square.y]
      return
    features = @_features[square.x][square.y].concat feature.index
    image = (do (@tileset.tiles[i].image for i in features).sort).join ' '
    if image not of PIXI.TextureCache
      @_features[square.x][square.y].length = 0
    @_features[square.x][square.y].push feature.index

  _set_feature_image: (square, image) ->
    if image?
      indices = (@tileset.tiles_by_image[x].index for x in image.split ' ')
      @_features[square.x][square.y] = indices
    else
      @_features[square.x][square.y].length = 0

  _set_tile_image: (square, image) ->
    @_tiles[square.x][square.y] = @tileset.tiles_by_image[image].index


class Stage
  HOTKEYS = 'qwertyu12345678'
  SCROLL_KEYS = {h: [-1, 0], j: [0, 1], k: [0, -1], l: [1, 0]}
  SCROLL_SPEED = 32

  constructor: ->
    @input = new base.Input {keyboard: true, mouse: true}
    @tileset = new Tileset
    @map = new Map @tileset, base.starting_map_uid
    @_graphics = new Graphics @
    @_hotkeys = {}
    @_set_hotkeys 0

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    if do @_maybe_scroll
      return
    keys = do @input.get_keys_pressed
    if 'a' of keys
      @_set_hotkeys @_hotkeys[0] + HOTKEYS.length
      @input.block_key 'a'
    target = @_graphics.get_target do @input.get_mouse_position
    if target.type != 'tile'
      return Session.set 'tilist.cursor', null
    Session.set 'tilist.cursor', target.outline
    for key of keys
      index = HOTKEYS.indexOf key
      if index >= 0 and @_hotkeys[index]?
        @_set_tile target.square, @tileset.tiles[@_hotkeys[index]]
      else if key == 's'
        if @map.clear_features target.square
          do @map.save

  _maybe_scroll: ->
    if @_scrolling and not do @_graphics.scroll
      return true
    @_scrolling = false
    for key of do @input.get_keys_pressed
      if key of SCROLL_KEYS
        offset = new Point SCROLL_KEYS[key][0], SCROLL_KEYS[key][1]
        @_graphics.prepare_scroll SCROLL_SPEED, offset
        @map = new Map @tileset, @map.get_uid offset
        @_scrolling = true
    @_scrolling

  _set_hotkeys: (index) ->
    if index > @tileset.tiles.length
      index = 0
    hotkeys = []
    for key, i in HOTKEYS
      if i + index < @tileset.tiles.length
        @_hotkeys[i] = i + index
        square = @_graphics.get_tileset_square @_hotkeys[i]
        hotkeys.push @_graphics.get_outline square
        hotkeys[hotkeys.length - 1].text = do key.toUpperCase
      else
        delete @_hotkeys[i]
    Session.set 'tilist.hotkeys', hotkeys

  _set_tile: (square, tile) ->
    changed = false
    if tile.tree_offset?
      [x, y] = tile.tree_offset
      top_left = square.subtract new Point x, y
      for [suffix, x, y] in @tileset.tree_data
        tree = @tileset.tiles_by_image["tree-#{suffix}"]
        changed = (@map.set_tile (top_left.add new Point x, y), tree) or changed
    else
      changed = @map.set_tile square, tile
    if changed
      do @map.save


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
      {image: 'tree-ul', tree_offset: [0, 0]}
      {image: 'tree-ur', tree_offset: [1, 0]}
      {image: 'tree-dl', tree_offset: [0, 1], constraint: is_yellow}
      {image: 'tree-dr', tree_offset: [1, 1], constraint: is_yellow}
      {image: 'castle-dome-l'}
      {image: 'castle-dome-r'}
      {image: 'castle-side-l'}
      {image: 'castle-side-r'}
      {image: 'castle-bricks'}
      {image: 'castle-cells'}
      {image: 'castle-mixed'}
      {image: 'castle-door'}
      {image: 'castle-roof-l'}
      {image: 'castle-roof-m'}
      {image: 'castle-roof-r'}
    ]
    @tree_data = [['ul', 0, 0], ['ur', 1, 0], ['dl', 0, 1], ['dr', 1, 1]]
    for feature in features
      feature.feature = true
      @tiles.push feature
    @tiles_by_image = {}
    for tile, i in @tiles
      tile.index = i
      @tiles_by_image[tile.image] = tile
    @default_tile = @tiles_by_image['grass-green-']
    # A constraint is valid if at least one of the fallback tiles satisfies it.
    @_fallback_tiles = [@default_tile, @tiles_by_image['grass-yellow-']]
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
  base.modes.tilist = Stage
