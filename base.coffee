@base = @base or {}

base.grid_in_pixels = 16
base.mode = 'main'


class base.Graphics
  constructor: (@stage, @element, options) ->
    # Graphics currently supports the following options:
    # - assets: List of asset files to load
    # - size: Point that controls the size of the surface, in grid cells
    # - transparent: Set to true to make the surface transparent
    options = options or {}
    @scale = options.scale or 2
    @size = (options.size or @stage.map.size).scale @scale*base.grid_in_pixels

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    renderer_options = {transparent: options?.transparent}
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y, renderer_options
    @element.prepend @renderer.view

    @context = new PIXI.Stage 0x00000000
    @tile_container = do @_add_container
    @feature_container = do @_add_container
    do @_initialize_stats

    @tiles = []
    @features = []

    assets = options.assets or ['effects', 'enemies', 'player', 'tileset']
    loader = new PIXI.AssetLoader ("#{asset}.json" for asset in assets)
    loader.onComplete = @_on_assets_loaded.bind @
    do loader.load

  draw: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        index = x*@stage.map.size.y + y
        square = new Point x, y
        @_draw_tile @tiles[index], @stage.map.get_tile_image square
        @_draw_tile @features[index], @stage.map.get_feature_image square
    @renderer.render @context

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

  _initialize_stats: ->
    @stats = new PIXI.Stats
    $('body').append @stats.domElement
    $(@stats.domElement).css {position: 'fixed', top: 0, left: 0}

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
