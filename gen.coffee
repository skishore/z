class Graphics extends base.Graphics
  constructor: (@stage) ->
    super @stage, {scale: 4, size: new Point 1, 1}
    @size = @stage.map.size.scale @scale
    @size = (@size.add @stage.map.num_rooms).add new Point 1, 1
    @renderer.resize @size

  draw: ->
    do @renderer.draw

  draw_map: (map) ->
    do @_clear_map
    for x in [0...map.size.x]
      for y in [0...map.size.y]
        color = map.get_pixel x, y
        dx = Math.floor x/@stage.map.room_size.x + 1
        dy = Math.floor y/@stage.map.room_size.y + 1
        @pixels.lineStyle 1, color, 1
        @pixels.beginFill color, 1
        @pixels.drawRect @scale*x + dx, @scale*y + dy, @scale - 1, @scale - 1
        do @pixels.endFill
    @pixels.lineStyle 1, 0x000000, 1
    room = map.room_size.scale @scale
    for x in [0...map.num_rooms.x]
      for y in [0...map.num_rooms.y]
        @pixels.drawRect x*(room.x + 1), y*(room.y + 1), room.x + 1, room.y + 1

  _clear_map: ->
    if @pixels?
      @layers.game.removeChild @pixels
    @pixels = new PIXI.Graphics
    @layers.game.addChild @pixels

  _on_assets_loaded: ->
    @element.height @size.y
    do @stage.loop.bind @stage


class Map
  COLORS = [0x0000ff, 0xccff00, 0x66ff66, 0x888800, 0x444400]
  GRADIENT = 1.2
  NUM_ROOMS = [7, 7]
  ROOM_SIZE = [18, 11]
  WAVELENGTH = 0.5

  constructor: ->
    @num_rooms = @_to_point NUM_ROOMS
    @room_size = @_to_point ROOM_SIZE
    @size = new Point @num_rooms.x*@room_size.x, @num_rooms.y*@room_size.y
    @wavelength = WAVELENGTH*@room_size.y
    do @_generate_level

  get_pixel: (x, y) ->
    COLORS[@tiles[x][y]]

  _clamp: (value, min, max) ->
    Math.min (Math.max value, min), max

  _cone: (square, values) ->
    diff = (square.subtract new Point @size.x/2, @size.y)
    values[square.x][square.y] + 2*(do diff.length)/@size.y

  _draw_river: (values) ->
    # The river starts at the lowest point in the top quarter of the map.
    start = new Point 0, 0
    for x in [0...@size.x]
      for y in [0...Math.floor @size.y/4]
        if values[x][y] < values[start.x][start.y]
          start = new Point x, y
    # Draw the river by, at each step, adding the lowest point not already
    # in the river to it.
    path = []
    neighbors = [start]
    visited = new PointSet
    visited.insert start
    steps = [(new Point 1, 0), (new Point -1, 0), \
             (new Point 0, 1), (new Point 0, -1)]
    while true
      # Pull out the neighbor at the lowest height.
      assert neighbors.length > 0
      best_node = neighbors[0]
      for node in neighbors
        if (@_cone node, values) < (@_cone best_node, values)
          best_node = node
      neighbors = _.without neighbors, best_node
      path.push best_node
      if best_node.y == @size.y - 1
        break
      # Push any new neighbors of the node onto the stack.
      for step in steps
        neighbor = best_node.add step
        if 0 <= neighbor.x < @size.x and 0 <= neighbor.y < @size.y
          if not visited.contains neighbor
            visited.insert neighbor
            neighbors.push neighbor
    path

  _generate_level: ->
    noise.seed do (new Date).getTime
    @tiles = ((0 for y in [0...@size.y]) for x in [0...@size.x])
    values = (((@_value x, y) for y in [0...@size.y]) for x in [0...@size.x])
    river = @_draw_river values
    for x in [0...@size.x]
      for y in [0...@size.y]
        @tiles[x][y] = @_threshold @_clamp values[x][y], 0, 1
    for square in river
      @tiles[square.x][square.y] = 0
      if square.x > 0
        @tiles[square.x - 1][square.y] = 0
      if square.y > 0
        @tiles[square.x][square.y - 1] = 0

  _threshold: (value) ->
    n = COLORS.length
    Math.min (Math.floor n*value), n - 1

  _to_point: (pair) ->
    new Point pair[0], pair[1]

  _value: (x, y) ->
    perlin = noise.perlin2 x/@wavelength, y/@wavelength
    ((perlin + GRADIENT*(0.5 - y/@size.y)) + 1)/2


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
    if 'r' of do @input.get_keys_pressed
      delete @_map_drawn
      @map = new Map
      @input.block_key 'r'
    if not @_map_drawn
      @_graphics.draw_map @map
      @_map_drawn = true


if Meteor.isClient
  base.modes.gen = Stage
