class Graphics extends base.Graphics
  constructor: (@stage) ->
    super @stage, {scale: 4, size: new Point 1, 1}
    @renderer.resize @stage.map.size.scale @scale
    @pixels = new PIXI.Graphics
    @layers.game.addChild @pixels

  draw: ->
    do @renderer.draw

  draw_map: (map) ->
    for x in [0...map.size.x]
      for y in [0...map.size.y]
        color = map.get_pixel x, y
        @pixels.lineStyle 1, color, 1
        @pixels.beginFill color, 1
        @pixels.drawRect @scale*x, @scale*y, @scale, @scale
        do @pixels.endFill
    @pixels.lineStyle 1, 0x000000, 1
    room = map.room_size.scale @scale
    for x in [0...map.num_rooms.x]
      for y in [0...map.num_rooms.y]
        @pixels.drawRect x*room.x, y*room.y, room.x - 1, room.y - 1

  _on_assets_loaded: ->
    @element.height @scale*@stage.map.size.y
    do @stage.loop.bind @stage


class Map
  GRADIENT = 1
  NUM_ROOMS = [7, 7]
  ROOM_SIZE = [18, 11]
  WAVELENGTH = 0.5

  constructor: ->
    @num_rooms = @_to_point NUM_ROOMS
    @room_size = @_to_point ROOM_SIZE
    @size = new Point @num_rooms.x*@room_size.x, @num_rooms.y*@room_size.y
    @wavelength = WAVELENGTH*@room_size.y

  get_pixel: (x, y) ->
    value = @_get_value x, y
    coordinate = Math.round 0xff*@_clamp (value + 1)/2, 0, 1
    coordinate + (coordinate << 8) + (coordinate << 16)

  _clamp: (value, min, max) ->
    Math.min (Math.max value, min), max

  _get_value: (x, y) ->
    value = -Infinity
    for dx in [0]
      for dy in [0]
        probe = noise.perlin2 (x + dx)/@wavelength, (y + dy)/@wavelength
        value = Math.max probe, value
    value + GRADIENT*(0.5 - y/@size.y)

  _to_point: (pair) ->
    new Point pair[0], pair[1]


class Stage
  constructor: ->
    @input = new base.Input {keyboard: true}
    @map = new Map
    @_graphics = new Graphics @

  loop: ->
    do @_graphics.stats.begin
    do @update
    do @_graphics.draw
    window.requestAnimationFrame @loop.bind @
    do @_graphics.stats.end

  update: ->
    if not @_map_drawn
      @_graphics.draw_map @map
      @_map_drawn = true


if Meteor.isClient
  base.modes.gen = Stage
