@gen = @gen or {}


_point = (pair) -> new Point pair[0], pair[1]

gen.KING_MOVES = _.map [[1, 0], [1, 1], [0, 1], [-1, 1],
                        [-1, 0], [-1, -1], [0, -1], [1, -1]], _point

gen.ROOK_MOVES = _.map [[1, 0], [-1, 0], [0, 1], [0, -1]], _point

gen.construct_2d_array = (size, value) ->
  ((value for y in [0...size.y]) for x in [0...size.x])

gen.Tile = {FREE: 0, BLOCKED: 1, HAZARD: 2, WALL: 3}


class gen.Level
  constructor: (@size) ->
    @tiles = gen.construct_2d_array @size, gen.Tile.FREE
    @protected = gen.construct_2d_array @size, false

  # Runs an erosion step on the level. Each tile has a chance of being converted
  # to the types of the tiles around it.
  erode: (options) ->
    min_neighbors = if options.min_neighbors? then options.min_neighbors else 4
    max_neighbors = if options.max_neighbors? then options.max_neighbors else 10
    if options.initial_seed?
      seed = options.initial_seed.seed
      noise.seed if seed? then seed else do (new Date).getTime

    new_tiles = _.map @tiles, _.clone
    steps = gen.KING_MOVES.concat new Point 0, 0
    for x in [1...@size.x - 1]
      for y in [1...@size.y - 1]
        if @protected[x][y] or not _can_erode_square new_tiles, new Point x, y
          continue
        if options.initial_seed?
          threshold = options.initial_seed.threshold or 0
          wavelength = options.initial_seed.wavelength or 1
          blocked = (noise.perlin2 x/wavelength, y/wavelength) > threshold
        else
          neighbors_blocked = 0
          neighbors_unknown = 0
          for step in steps
            if @tiles[x + step.x][y + step.y] == gen.Tile.BLOCKED
              neighbors_blocked += 1
            else if @tiles[x + step.x][y + step.y] != gen.Tile.FREE
              neighbors_unknown += 1
          discriminant = neighbors_blocked + neighbors_unknown/2
          blocked = min_neighbors < discriminant < max_neighbors
        new_tiles[x][y] = if blocked then gen.Tile.BLOCKED else gen.Tile.FREE
    @tiles = new_tiles

  # Returns a human-readable serialization of the level.
  to_debug_string: (show_rooms) ->
    result = ''
    for y in [0...@size.y]
      row = '\n'
      for x in [0...@size.x]
        edge = y == 0 or y == @size.y - 1
        row += _get_debug_char_for_tile @tiles[x][y], edge
      result += row
    result


# Returns true if it is possible to erode the given square in the map.
# A square is immune to erosion if:
#  - It has no free orthogonal neighbors. We want the map to be connected.
#  - Eroding it would change the orthogonal connectivity of tiles around it.
_can_erode_square = (tiles, square) ->
  if tiles[square.x][square.y] not in [gen.Tile.FREE, gen.Tile.BLOCKED]
    return false
  has_free_orthogonal_neighbor = false
  min_unblocked_index = -1
  max_unblocked_index = -1
  gaps = 0
  for i in [0...8]
    step = gen.KING_MOVES[i]
    if tiles[square.x + step.x][square.y + step.y] != gen.Tile.FREE
      continue
    has_free_orthogonal_neighbor |= (i % 2 == 0)
    if min_unblocked_index < 0
      min_unblocked_index = i
      max_unblocked_index = i
      continue
    if i > max_unblocked_index + 1
      gaps += 1
    max_unblocked_index = i
  if (min_unblocked_index >= 0 and
      not (min_unblocked_index == 0 and max_unblocked_index == 7))
    gaps += 1
  return gaps <= 1 && has_free_orthogonal_neighbor

_get_debug_char_for_tile = (tile, edge) ->
  result = '.#{|'[tile]
  if result == '|' and edge
    result = '-'
  if result? then result else '?'

_in_bounds = (point, size) ->
  0 <= point.x < size.x and 0 <= point.y < size.y

_is_tile_blocked = (tile) ->
  tile == gen.Tile.DEFAULT or tile == gen.Tile.WALL
