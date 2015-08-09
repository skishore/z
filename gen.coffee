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
  NUM_ROOMS = [3, 3]
  ROOM_SIZE = [18, 11]
  WAVELENGTH = 0.4

  constructor: ->
    @num_rooms = @_to_point NUM_ROOMS
    @room_size = @_to_point ROOM_SIZE
    @size = new Point @num_rooms.x*@room_size.x, @num_rooms.y*@room_size.y
    @wavelength = WAVELENGTH*@room_size.y
    do @_generate_level

  get_pixel: (x, y) ->
    COLORS[@tiles[x][y]]

  save: ->
    tileset = new base.gen.tileset
    dx = Math.floor @num_rooms.x/2
    dy = Math.floor @num_rooms.y/2
    for x in [0...@num_rooms.x]
      for y in [0...@num_rooms.y]
        uid = {zone: 'perlin', position: {x: x - dx, y: y - dy}}
        map = new base.gen.map tileset, uid
        @_write_map map, new Point x, y
        do map.save

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

  _extrude: (tiles) ->
    result = ((0 for y in [0...@size.y]) for x in [0...@size.x])
    for x in [0...@size.x]
      for y in [0...@size.y]
        dx = Math.floor x/@room_size.x
        dy = Math.floor y/@room_size.y
        result[x][y] = tiles[x - dx][y - dy]
    result

  _generate_level: ->
    # Decrease the size of the map by 1 so that room edges will overlap.
    @size.x -= @num_rooms.x - 1
    @size.y -= @num_rooms.y - 1
    # Compute the set of tiles on the shrunken map.
    @seed = do (new Date).getTime
    noise.seed @seed
    @tiles = ((0 for y in [0...@size.y]) for x in [0...@size.x])
    values = (((@_value x, y) for y in [0...@size.y]) for x in [0...@size.x])
    river = @_draw_river values
    for x in [0...@size.x]
      for y in [0...@size.y]
        @tiles[x][y] = @_threshold values[x][y]
    for square in river
      @tiles[square.x][square.y] = 0
    # Return the map to the normal size, repeating tiles at room edges.
    @size.x += @num_rooms.x - 1
    @size.y += @num_rooms.y - 1
    @tiles = @_extrude @tiles

  _threshold: (value) ->
    n = COLORS.length
    @_clamp (Math.floor n*value), 0, n - 1

  _to_point: (pair) ->
    new Point pair[0], pair[1]

  _value: (x, y) ->
    perlin = noise.perlin2 x/@wavelength, y/@wavelength
    ((perlin + GRADIENT*(0.5 - y/@size.y)) + 1)/2

  _write_map: (map, room) ->
    TILES = [undefined, ['bush', 'flower'], ['water'], ['grass-yellow-']]
    tbi = map.tileset.tiles_by_image
    options = {
      river: {
        centrality: (2*do Math.random) + 1
        randomness: (2*do Math.random) + 1
        windiness: (2*do Math.random) + 1
        length: (do Math.random) - 1
      }
      erode: {
        initial_seed: {
          threshold: -0.2
          wavelength: 4
        }
        num_iterations: 1
      }
    }
    river_map = new gen.RiverMap \
        @room_size, (new Point 0, (_.random 2, @room_size.y - 3)), \
        (new Point (_.random 2, @room_size.x - 3), @room_size.y - 1), options
    for x in [0...@room_size.x]
      for y in [0...@room_size.y]
        map.set_tile (new Point x, y), tbi['grass-green-']
    wavelength = 4
    noise.seed do (new Date).getTime
    for x in [0...@room_size.x]
      for y in [0...@room_size.y]
        tiles = TILES[river_map.tiles[x][y]]
        if not tiles?
          continue
        bit = (noise.perlin2 x/wavelength, y/wavelength) > 0
        tile = tiles[(x + y + (if bit then 1 else 0)) % tiles.length]
        map.set_tile (new Point x, y), tbi[tile]
    for x in [0...@room_size.x]
      for y in [0...@room_size.y]
        square = new Point x, y
        if (map.get_tile_image square) == 'grass-green-udrl'
          map.set_tile square, tbi['grass-yellow-']
    # Build the trees around the map edges.
    # TODO(skishore): This method is a terrible hack that hardcodes room size.
    tree_ul = tbi['tree-ul']
    tree_ur = tbi['tree-ur']
    tree_dl = tbi['tree-dl']
    tree_dr = tbi['tree-dr']
    y = @room_size.y - 1
    for x in [2...8].concat [10...16]
      map.set_tile (new Point x, 0), if x % 2 == 0 then tree_dl else tree_dr
      map.set_tile (new Point x, y), if x % 2 == 0 then tree_ul else tree_ur
    x = @room_size.x - 1
    for y in [1...5].concat [6...10]
      down = (y - (y > 5)) % 2 == 0
      map.set_tile (new Point 0, y), if down then tree_dr else tree_ur
      map.set_tile (new Point x, y), if down then tree_dl else tree_ul
    for x in [0, @room_size.x - 2]
      for y in [0, @room_size.y - 2]
        map.set_tile (new Point x, y), tree_ul
        map.set_tile (new Point x + 1, y), tree_ur
        map.set_tile (new Point x, y + 1), tree_dl
        map.set_tile (new Point x + 1, y + 1), tree_dr
    # Block off impassible exits with rocks.
    for x in [8, 9]
      for y in [0, @room_size.y - 1]
        at_x_edge = room.x*@room_size.x + x in [0, @size.x - 1]
        at_y_edge = room.y*@room_size.y + y in [0, @size.y - 1]
        tile = @tiles[room.x*@room_size.x + x][room.y*@room_size.y + y]
        if tile == 0 or at_x_edge or at_y_edge
          map.set_tile (new Point x, y), tbi['rock']
    for x in [0, @room_size.x - 1]
      for y in [5]
        at_x_edge = room.x*@room_size.x + x in [0, @size.x - 1]
        at_y_edge = room.y*@room_size.y + y in [0, @size.y - 1]
        tile = @tiles[room.x*@room_size.x + x][room.y*@room_size.y + y]
        if tile == 0 or at_x_edge or at_y_edge
          map.set_tile (new Point x, y), tbi['rock']


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
      do @map.save


if Meteor.isClient
  base.modes.gen = Stage
