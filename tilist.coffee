class Graphics
  GRID_IN_PIXELS = 16

  constructor: (@stage, @element, callback) ->
    num_tiles = @stage.map.tileset.tiles.length
    size = do @stage.map.size.clone
    size.y += (Math.ceil num_tiles/size.x) + 1

    @scale = 2
    @size = size.scale @scale*GRID_IN_PIXELS
    @tiles = []
    @tileset = []

    PIXI.scaleModes.DEFAULT = PIXI.scaleModes.NEAREST
    @renderer = PIXI.autoDetectRenderer @size.x, @size.y, {transparent: true}
    @element.prepend @renderer.view

    @context = new PIXI.Stage 0x00000000
    @map_container = do @_add_container
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

  _on_assets_loaded: ->
    size = @stage.map.size
    for x in [0...size.x]
      for y in [0...size.y]
        image = (@stage.map.get_tile new Point x, y).image
        tile = new PIXI.Sprite PIXI.Texture.fromFrame image
        tile.x = GRID_IN_PIXELS*x
        tile.y = GRID_IN_PIXELS*y
        @tiles.push tile
        @map_container.addChild tile
    for choice, i in @stage.map.tileset.tiles
      tile = new PIXI.Sprite PIXI.Texture.fromFrame choice.image
      tile.x = GRID_IN_PIXELS*(i % size.x)
      tile.y = GRID_IN_PIXELS*(size.y + (Math.floor i/size.x) + 1)
      @tileset.push tile
      @map_container.addChild tile
    do @stage.loop.bind @stage

  draw: ->
    @renderer.render @context

  get_target: (position) ->
    offset = do @element.offset
    position = do position.clone
    position.x -= offset.left
    position.y -= offset.top
    grid = @scale*GRID_IN_PIXELS
    square = new Point \
        (Math.floor position.x/(@scale*GRID_IN_PIXELS)),
        (Math.floor position.y/(@scale*GRID_IN_PIXELS))
    outline = {
      left: square.x*grid
      top: square.y*grid
      width: grid
      height: grid
    }
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
  constructor: (@size, @tileset) ->
    assert @size.x > 0 and @size.y > 0
    @_tiles = (@tileset.default_tile.index for i in [0...@size.x*@size.y])

  get_tile: (square) ->
    window.map = @
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      return @tileset.tiles[@_tiles[square.x*@size.y + square.y]]
    @tileset.default_tile

  _set_tile: (square, tile) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      @_tiles[square.x*@size.y + square.y] = tile.index


class Stage
  constructor: ->
    @input = new Input
    @map = new Map (new Point 18, 11), new Tileset
    @_graphics = new Graphics @, $('.surface')

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    target = @_graphics.get_target do @input.get_mouse_position
    Session.set 'cursor', target.outline


class Tileset
  constructor: ->
    @tiles = [
      {image: 'grass-green-', edging: (t) -> t.image.startsWith 'grass-green'}
      {image: 'grass-green-flat'}
      {image: 'grass-green-flower'}
      {image: 'grass-green-tall'}
      {image: 'grass-yellow-', edging: (t) -> t.image != 'water'}
      {image: 'grass-yellow-flat'}
      {image: 'bush'}
      {image: 'flower'}
      {image: 'rock'}
      {image: 'sign'}
      {image: 'tree-ul', tree: true}
      {image: 'tree-ur', tree: true}
      {image: 'tree-dl', tree: true}
      {image: 'tree-dr', tree: true}
      {image: 'tree-ul-dr', tree: true}
      {image: 'tree-ur-dl', tree: true}
      {image: 'water'}
    ]
    @tiles_by_image = {}
    for tile, i in @tiles
      tile.index = i
      @tiles_by_image[tile.image] = tile
    @default_tile = @tiles[0]


if Meteor.isClient
  Template.tilist.helpers {
    has_cursor: -> (Session.get 'cursor')?
    cursor: -> Session.get 'cursor'
  }
  Meteor.startup (-> stage = new Stage)
