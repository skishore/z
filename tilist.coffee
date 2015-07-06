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
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        image = @stage.map.get_image new Point x, y
        tile = new PIXI.Sprite PIXI.Texture.fromFrame image
        tile.x = GRID_IN_PIXELS*x
        tile.y = GRID_IN_PIXELS*y
        tile._tilist_image = image
        @tiles.push tile
        @map_container.addChild tile
    for choice, i in @stage.map.tileset.tiles
      square = @get_tileset_square i
      tile = new PIXI.Sprite PIXI.Texture.fromFrame choice.image
      tile.x = GRID_IN_PIXELS*square.x
      tile.y = GRID_IN_PIXELS*square.y
      @tileset.push tile
      @map_container.addChild tile
    do @stage.loop.bind @stage

  draw: ->
    for x in [0...@stage.map.size.x]
      for y in [0...@stage.map.size.y]
        image = @stage.map.get_image new Point x, y
        tile = @tiles[x*@stage.map.size.y + y]
        if tile._tilist_image != image
          tile.setTexture PIXI.Texture.fromFrame image
          tile._tilist_image = image
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
    @_tiles = (@tileset.default_tile.index for i in [0...@size.x*@size.y])

  get_image: (square) ->
    tile = @get_tile square
    image = tile.image
    if tile.edging?
      for [direction, x, y] in EDGES
        other = @get_tile square.add new Point x, y
        if not tile.edging other
          image += direction
    image

  get_tile: (square) ->
    window.map = @
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      return @tileset.tiles[@_tiles[square.x*@size.y + square.y]]
    @tileset.default_tile

  set_tile: (square, tile) ->
    if 0 <= square.x < @size.x and 0 <= square.y < @size.y
      @_tiles[square.x*@size.y + square.y] = tile.index


class Stage
  HOTKEYS = ['q', 'w', 'e', 'r']

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
    if target.type == 'tile'
      Session.set 'tilist.cursor', target.outline
    else
      Session.set 'tilist.cursor', null
    for key of do @input.get_keys_pressed
      index = '1234567890'.indexOf key
      if index >= 0
        for i in [0...@_num_hotkeys]
          @_set_hotkey i, index*@_num_hotkeys + i
        do @_redraw_hotkeys
        @input.block_key key
      index = HOTKEYS.indexOf key
      if index >= 0 and target.type == 'tile' and @_hotkeys[index]?
        map.set_tile target.square, @map.tileset.tiles[@_hotkeys[index]]

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
    @tiles = [
      {image: 'grass-green-', edging: (t) -> t.image.startsWith 'grass-green'}
      {image: 'grass-green-flat'}
      {image: 'grass-green-flower'}
      {image: 'grass-green-tall'}
      {image: 'bush'}
      {image: 'flower'}
      {image: 'rock'}
      {image: 'sign'}
      {image: 'tree-ul', tree: true}
      {image: 'tree-ur', tree: true}
      {image: 'tree-dl', tree: true}
      {image: 'tree-dr', tree: true}
      {image: 'grass-yellow-', edging: (t) -> t.image != 'water'}
      {image: 'grass-yellow-flat'}
      {image: 'water'}
    ]
    @tiles_by_image = {}
    for tile, i in @tiles
      tile.index = i
      @tiles_by_image[tile.image] = tile
    @default_tile = @tiles[0]


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
