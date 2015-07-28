@base = @base or {}

base.collection = new Meteor.Collection 'maps'
base.door_animation_frames = 4
base.gen = {}
base.grid_in_pixels = 16
base.map_size = [18, 11]
base.modes = {}
base.starting_map_uid = {zone: 'garden', position: {x: 0, y: 0}}
base.starting_mode = 'gen'

if Meteor.isServer
  Meteor.publish 'maps', -> do base.collection.find


class Renderer
  LAYERS = ['game', 'ui']
  singleton = null

  constructor: (callback) ->
    PIXI.SCALE_MODES.DEFAULT = PIXI.SCALE_MODES.NEAREST
    @_renderer = PIXI.autoDetectRenderer 0, 0, {transparent: true}
    @_container = new PIXI.Container
    @element = $('.game-wrapper .surface')
    @layers = {}
    for layer in LAYERS
      @layers[layer] = new PIXI.Container
      @_container.addChild @layers[layer]
    @element.prepend @_renderer.view
    do @_initialize_stats
    # Load assets and run the callback when done.
    for asset in ['doors', 'effects', 'enemies', 'player', 'tileset']
      PIXI.loader.add asset, "#{asset}.json"
    PIXI.loader.once 'complete', callback
    do PIXI.loader.load

  draw: ->
    @_renderer.render @_container

  generate_texture: ->
    @layers.game.generateTexture @_renderer, 1, PIXI.SCALE_MODES.NEAREST

  resize: (size) ->
    @_renderer.resize size.x, size.y

  @get_singleton: (callback) ->
    if singleton?
      Meteor.setTimeout callback
    else
      singleton = new Renderer callback
    singleton

  _initialize_stats: ->
    @stats = new PIXI.Stats
    $('body').append @stats.domElement
    $(@stats.domElement).css {position: 'fixed', top: 0, left: 0}


class base.Graphics
  constructor: (@stage, options) ->
    # Graphics currently supports the following options:
    # - scale: Scale factor for any bitmaps drawn on the surface.
    # - size: Point that controls the size of the surface, in grid cells
    options = options or {}
    @scale = options.scale or 2
    @size = (options.size or @stage.map.size).scale @scale*base.grid_in_pixels
    @renderer = Renderer.get_singleton @_on_assets_loaded.bind @
    @renderer.resize @size
    @element = @renderer.element
    @layers = @renderer.layers
    @stats = @renderer.stats
    # Containers for the various layers of the game. Earlier layers are lower.
    @tile_container = @_add_container @layers.game
    @feature_container = @_add_container @layers.game
    @tiles = []
    @features = []

  draw: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        index = x*@stage.map.size.y + y
        square = new Point x, y
        @_draw_tile @tiles[index], @stage.map.get_tile_image square
        @_draw_tile @features[index], @stage.map.get_feature_image square
    do @renderer.draw

  prepare_scroll: (speed, diff) ->
    assert not @_scroll?
    assert not do diff.zero
    size = @stage.map.size.scale @scale*base.grid_in_pixels
    offset = new Point diff.x*size.x, diff.y*size.y
    # When we call generateTexture, the texture may be slightly larger than
    # the expected size to accomodate any sprites that are outside the game's
    # normal bounds. Shift it to account for this fact.
    bounds = do @layers.game.getBounds
    sprite = new PIXI.Sprite do @renderer.generate_texture
    sprite.x = -offset.x + bounds.x
    sprite.y = -offset.y + bounds.y
    @layers.game.addChildAt sprite, 0
    @_scroll = {
      cur_frame: -1
      max_frame: Math.ceil (do offset.length)/speed
      offset: offset
      sprite: sprite
    }
    assert not do @scroll

  scroll: ->
    if not @_scroll?
      return true
    @_scroll.cur_frame += 1
    factor = (@_scroll.max_frame - @_scroll.cur_frame)/@_scroll.max_frame
    @layers.game.x = Math.floor @_scroll.offset.x*factor
    @layers.game.y = Math.floor @_scroll.offset.y*factor
    # Destroy the texture and return true if the scroll is complete.
    complete = @_scroll.cur_frame >= @_scroll.max_frame
    if complete
      @layers.game.removeChild @_scroll.sprite
      @_scroll.sprite.texture.destroy true
      delete @_scroll
    complete

  _add_container: (layer) ->
    container = new PIXI.Container
    container.scale = new PIXI.Point @scale, @scale
    layer.addChild container
    container

  _draw_tile: (tile, image) ->
    if tile._tilist_image != image
      tile.texture = PIXI.utils.TextureCache[image] or PIXI.Texture.EMPTY
      tile._tilist_image = image

  _make_tile: (square, image, container) ->
    tile = new PIXI.Sprite
    tile.x = base.grid_in_pixels*square.x
    tile.y = base.grid_in_pixels*square.y
    @_draw_tile tile, image
    container.addChild tile
    tile

  _on_assets_loaded: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        square = new Point x, y
        tile = @stage.map.get_tile_image square
        feature = @stage.map.get_feature_image square
        @tiles.push @_make_tile square, tile, @tile_container
        @features.push @_make_tile square, feature, @feature_container
    # For some reason, canvas elements take up extra height by default.
    @element.height @size.y
    do @stage.loop.bind @stage


class base.Input
  constructor: (options) ->
    # Input currently supports the following options:
    # - keyboard - Set to true to handle keyboard input
    # - mouse - Set to true to handle mouse input
    options = options or {}
    if options.keyboard
      window.onkeydown = @_onkeydown.bind @
      window.onkeyup = @_onkeyup.bind @
    if options.mouse
      window.onmousemove = @_onmousemove.bind @
    @_blocked = {}
    @_pressed = {}
    @_mouse_position = new Point 0, 0

  block_key: (key) ->
    @_blocked[key] = true

  get_keys_pressed: ->
    _.fast_omit @_pressed, @_blocked

  get_mouse_position: ->
    do @_mouse_position.clone

  _get_key: (e) ->
    key = String.fromCharCode e.which
    if not e.shiftKey
      key = do key.toLowerCase
    key

  _onkeydown: (e) ->
    key = @_get_key e
    # TODO(skishore): Move dialog logic into a stage state in main.coffee.
    if not DialogManager.on_input key
      @_pressed[key] = true

  _onkeyup: (e) ->
    key = @_get_key e
    delete @_blocked[key]
    delete @_pressed[key]

  _onmousemove: (e) ->
    @_mouse_position.x = e.x
    @_mouse_position.y = e.y


class base.Map
  constructor: (uid, options) ->
    # Map currently supports the following options:
    # - raw: If true, the map data will be loaded from its raw values.
    options = options or {}
    @size = new Point base.map_size[0], base.map_size[1]
    @_tiles = @_tiles or do @_construct_2d_array
    @_features = @_features or do @_construct_2d_array
    @load uid, options.raw

  get_feature_image: (square) ->
    @_features[square.x][square.y]

  get_tile_image: (square) ->
    @_tiles[square.x][square.y]

  get_uid: (offset) ->
    pos = @uid.position
    {zone: @uid.zone, position: {x: pos.x + offset.x, y: pos.y + offset.y}}

  load: (uid, raw) ->
    name = @_uid_to_name uid
    document = base.collection.findOne {name: name}
    if not document?
      document = @_build_default_document uid
    @uid = document.uid
    if raw and document.raw?
      @_tiles = document.raw.tiles
      @_features = document.raw.features
      return
    for x in [0...@size.x]
      for y in [0...@size.y]
        square = new Point x, y
        @_set_tile_image square, document.tiles[x][y]
        @_set_feature_image square, document.features[x][y]

  save: ->
    document = @_build_default_document @uid
    for x in [0...@size.x]
      for y in [0...@size.y]
        square = new Point x, y
        document.tiles[x][y] = @get_tile_image square
        document.features[x][y] = @get_feature_image square
    document.raw = {tiles: @_tiles, features: @_features}
    id = (base.collection.findOne {name: document.name})?._id
    if id?
      base.collection.update {_id: id}, {$set: document}
    else
      base.collection.insert document

  _build_default_document: (uid) ->
    uid: uid
    name: @_uid_to_name uid
    tiles: @_construct_2d_array 'grass-green-'
    features: @_construct_2d_array undefined

  _construct_2d_array: (value) ->
    (((_.clone value) for y in [0...@size.y]) for x in [0...@size.x])

  _in_bounds: (square) ->
    0 <= square.x < @size.x and 0 <= square.y < @size.y

  _set_feature_image: (square, image) ->
    assert @_in_bounds square
    @_features[square.x][square.y] = image

  _set_tile_image: (square, image) ->
    assert @_in_bounds square
    @_tiles[square.x][square.y] = image

  _uid_to_name: (uid) ->
    "#{uid.zone}.#{uid.position.x}.#{uid.position.y}"
