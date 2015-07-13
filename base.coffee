@base = @base or {}

base.collection = new Meteor.Collection 'maps'
base.grid_in_pixels = 16
base.map_size = [18, 11]
base.modes = {}

if Meteor.isServer
  Meteor.publish 'maps', -> do base.collection.find


class Renderer
  singleton = null

  constructor: (element, callback) ->
    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @context = new PIXI.Stage 0x00000000
    @_renderer = PIXI.autoDetectRenderer 0, 0, {transparent: true}
    element.prepend @_renderer.view
    do @_initialize_stats
    # Load assets and run the callback when done.
    assets = ['effects', 'enemies', 'player', 'tileset']
    loader = new PIXI.AssetLoader ("#{asset}.json" for asset in assets)
    loader.onComplete = callback
    do loader.load

  draw: ->
    @_renderer.render @context

  resize: (size) ->
    @_renderer.resize size.x, size.y

  @get_singleton: (element, callback) ->
    if singleton?
      Meteor.setTimeout callback
    else
      singleton = new Renderer element, callback
    singleton

  _initialize_stats: ->
    @stats = new PIXI.Stats
    $('body').append @stats.domElement
    $(@stats.domElement).css {position: 'fixed', top: 0, left: 0}


class base.Graphics
  constructor: (@stage, @element, options) ->
    # Graphics currently supports the following options:
    # - size: Point that controls the size of the surface, in grid cells
    options = options or {}
    @scale = options.scale or 2
    size = (options.size or @stage.map.size).scale @scale*base.grid_in_pixels
    @renderer = Renderer.get_singleton @element, @_on_assets_loaded.bind @
    @renderer.resize size
    @context = @renderer.context
    @stats = @renderer.stats
    # Containers for the various layers of the game. Earlier layers are lower.
    @tile_container = do @_add_container
    @feature_container = do @_add_container
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

  _add_container: ->
    container = new PIXI.DisplayObjectContainer
    container.scale = new PIXI.Point @scale, @scale
    @context.addChild container
    container

  _draw_tile: (tile, image) ->
    if tile._tilist_image != image
      if image?
        tile.setTexture PIXI.Texture.fromFrame image
      else
        tile.setTexture PIXI.Texture.emptyTexture
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
    @_pressed[@_get_key e] = true

  _onkeyup: (e) ->
    key = @_get_key e
    delete @_blocked[key]
    delete @_pressed[key]

  _onmousemove: (e) ->
    @_mouse_position.x = e.x
    @_mouse_position.y = e.y


class base.Map
  constructor: (name, options) ->
    # Map currently supports the following options:
    # - raw: If true, the map data will be loaded from its raw values.
    options = options or {}
    @size = new Point base.map_size[0], base.map_size[1]
    @_tiles = @_tiles or do @_construct_2d_array
    @_features = @_features or do @_construct_2d_array
    @load name, options.raw

  get_feature_image: (square) ->
    @_features[square.x][square.y]

  get_tile_image: (square) ->
    @_tiles[square.x][square.y]

  load: (name, raw) ->
    document = base.collection.findOne {name: name}
    if not document?
      document = @_build_default_document name
    if raw and document.raw?
      @_tiles = document.raw.tiles
      @_features = document.raw.features
      return
    for x in [0...@size.x]
      for y in [0...@size.y]
        square = new Point x, y
        @_set_tile_image square, document.tiles[x][y]
        @_set_feature_image square, document.features[x][y]

  save: (name) ->
    check name, String
    document = @_build_default_document name
    for x in [0...@size.x]
      for y in [0...@size.y]
        square = new Point x, y
        document.tiles[x][y] = @get_tile_image square
        document.features[x][y] = @get_feature_image square
    document.raw = {tiles: @_tiles, features: @_features}
    id = (base.collection.findOne {name: name})?._id
    if id?
      base.collection.update {_id: id}, {$set: document}
    else
      base.collection.insert document

  _build_default_document: (name) ->
    name: name
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
