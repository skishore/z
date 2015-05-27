@gen = @gen or {}


gen.construct_2d_array = (size, value) ->
  ((value for y in [0...size.y]) for x in [0...size.x])

gen.rect_to_rect_distance = (rect1, rect2) ->
  distance = new Point(
      Math.max(rect1.position.x - rect2.position.x - rect2.size.x,
               rect2.position.x - rect1.position.x - rect1.size.x, 0),
      Math.max(rect1.position.y - rect2.position.y - rect2.size.y,
               rect2.position.y - rect1.position.y - rect1.size.y, 0))
  do distance.length


gen.Tile = {DEFAULT: 0, FREE: 1, WALL: 2, DOOR: 3, FENCE: 4}


class gen.Rect
  constructor: (@size, @position) ->


class gen.Room
  constructor: ->
    @squares = []

  get_random_square: ->
    _.sample @squares


class gen.Level
  constructor: (@size) ->
    @tiles = gen.construct_2d_array @size, gen.Tile.DEFAULT
    @rids = gen.construct_2d_array @size, 0
    @diggable = gen.construct_2d_array @size, true

  # Turns any DEFAULT square adjacent to a FREE square into a wall.
  add_walls: ->
    for x in [0...@size.x]
      for y in [0...@size.y]
        if _is_tile_blocked @tiles[x][y]
          continue
        for move in _KING_MOVES
          square = new Point x + move.x, y + move.y
          if ((_in_bounds square, @size) and
              @tiles[square.x][square.y] == gen.Tile.DEFAULT)
            @tiles[square.x][square.y] = gen.Tile.WALL
 
  # Digs a corridor between the two rooms, modifying tiles and diggable.
  # Returns true if the corridor was successfully dug.
  #
  # Windiness is between 1.0 and 8.0, with increasing windiness causing the
  # corridor digger to take longer paths between rooms.
  dig_corridor: (rooms, index1, index2, windiness) ->
    [room1, room2] = [rooms[index1], rooms[index2]]
    source = do room1.get_random_square
    target = do room1.get_random_square
    assert (_in_bounds source, @size) and @diggable[source.x][source.y]
    assert (_in_bounds target, @size) and @diggable[target.x][target.y]

    visited = new PointSet
    parents = new PointMap
    distances = new PointMap
    distances.set source, 0

    # Run Djikstra's algorithm between the source and target.
    while distances.length > 0 and not visited.contains target
      best_distance = Infinity
      best_key = undefined
      for key in _.keys distances
        if distances[key] < best_distance
          best_distance = distance
          best_key = key
      assert best_key?
      delete distances[best_key]
      best_node = distances.extract best_key
      visited.insert best_node
      for step in _ROOK_MOVES
        child = Point.sum best_node, step
        if ((not (_in_bounds child, @size) and diggable[child.x][child.y]) or
            visited.contains child)
          continue
        blocked = _is_tile_blocked @tiles[child.x][child.y]
        distance = best_distance + (if blocked then 2.0 else windiness)
        distances.set child, distance
        parents.set child, best_node

    # We may have terminated without finding a path.
    if not visited.contains target
      return false

    # Construct the actual path from source to target.
    # Guarantee that the first element of the path is target and the last source.
    node = target
    path = [node]
    while not node.equals source
      node = parents.get node
      path.push node

    # Truncate the path to only include sections outside the two rooms.
    # Guarantee that the first element of the path is in r2 and the last in r1.
    truncated_path = []
    for node in path
      if @rids[node.x][node.y] == index2 + 1
        truncated_path = []
      truncated_path.push node
      if @rids[node.x][node.y] == index1 + 1
        break

    # Truncate the path further: remove nodes from the beginning until there
    # is exactly one vertex on the path with a rook neighbor in the second room.
    # Do the same at the end until there is exactly one vertex on the path with
    # a rook neighbor in the first room.
    assert truncated_path.length > 2
    neighbor_in_room = [undefined]
    while _has_neighbor_in_room(
          @rids, truncated_path[2], index2 + 1, neighbor_in_room)
      do truncated_path.shift
      do truncated_path.shift
      truncated_path.unshift neighbor_in_room[0]
    while _has_neighbor_in_room(
          @rids, truncated_path[truncated_path.length - 3],
          index1 + 1, neighbor_in_room)
      do truncated_path.pop
      do truncated_path.pop
      truncated_path.push neighbor_in_room[0]

    # Dig the corridor, but don't dig through doors.
    assert truncated_path.length > 2
    for i in [1...truncated_path.length - 1]
      node = truncated_path[i]
      if _is_tile_blocked @tiles[node.x][node.y]
        @tiles[node.x][node.y] = gen.Tile.FREE
    _add_door truncated_path[1], @tiles, @diggable
    _add_door(truncated_path[truncated_path.length - 2], @tiles, @diggable)
    true

  # Runs an erosion step on the level. Each tile has a chance of being
  # converted to the types of the tiles around it.
  #
  # Islandness is between 0 and 16, with increasing islandness causing it to
  # be more likely to have random walls in the interior of a room.
  erode: (islandness) ->
    new_rids = _.map @rids, _.clone
    for x in [1...@size.x - 1]
      for y in [1...@size.y - 1]
        room_index = [0]
        if not _can_erode_square new_rids, new Point(x, y), room_index
          continue
        neighbors_blocked = 0
        for step in _KING_MOVES
          if @rids[x + step.x][y + step.y] == 0
            neighbors_blocked += 1
        blocked = @rids[x][y] == 0
        matches = if blocked then neighbors_blocked else 8 - neighbors_blocked
        inverse_blocked_to_free = 2
        inverse_free_to_blocked = 4
        cutoff = Math.max 8 - matches, matches - 8 + islandness
        changed = false
        if blocked
            changed = (_.random 8*inverse_blocked_to_free - 1) < 8 - matches
        else
            changed = (_.random 8*inverse_free_to_blocked - 1) < cutoff
        if changed
          new_rids[x][y] = if blocked then room_index else 0
          @tiles[x][y] = if blocked then gen.Tile.FREE else gen.Tile.DEFAULT
    @rids = new_rids

  # Returns a rooms array filled with the final (non-rectangular) rooms.
  extract_final_rooms: (n) ->
    result = (new gen.Room for i in [0...n])
    for x in [0...@size.x]
      for y in [0...@size.y]
        room_index = @rids[x][y]
        assert (room_index == 0) == _is_tile_blocked @tiles[x][y]
        if room_index == 0
          continue
        assert room_index - 1 < n
        result[room_index - 1].squares.push new Point x, y
        for step in _BISHOP_MOVES
          neighbor = new Point x + step.x, y + step.y
          if _is_tile_blocked @tiles[neighbor.x][neighbor.y]
            adjacent_to_room = false
            for step_two in _ROOK_MOVES
              neighbor_two = Point.sum neighbor, step_two
              if ((_in_bounds neighbor_two, @size) and
                  room_index == @rids[neighbor_two.x][neighbor_two.y])
                adjacent_to_room = true
            if not adjacent_to_room
              @diggable[neighbor.x][neighbor.y] = false
    result
 
  # Returns true and adds rect to rects if the room was successfully placed.
  place_rectangular_room: (rect, separation, rects) ->
    for other in rects
      if (gen.rect_to_rect_distance rect, other) < separation
        return false
    room_index = rects.length + 1
    for x in [0...rect.size.x]
      for y in [0...rect.size.y]
        @tiles[x + rect.position.x][y + rect.position.y] = gen.Tile.FREE
        @rids[x + rect.position.x][y + rect.position.y] = room_index
    rects.push rect
    true

  # Returns a human-readable serialization of the level.
  to_debug_string: (show_rooms) ->
    result = ''
    for y in [0...@size.y]
      row = '\n'
      for x in [0...@size.x]
        if show_rooms && @rids[x][y] > 0
          row += '' + ((@rids[x][y] - 1) % 10)
        else
          row += _get_debug_char_for_tile @tiles[x][y]
      result += row
    result


_point = (pair) -> new Point pair[0], pair[1]

_BISHOP_MOVES = _.map [[1, 1], [1, -1], [-1, 1], [-1, -1]], _point

# IMPORTANT: The king moves are arranged in increasing order of angle.
_KING_MOVES = _.map [[1, 0], [1, 1], [0, 1], [-1, 1],
                     [-1, 0], [-1, -1], [0, -1], [1, -1]], _point

_ROOK_MOVES = _.map [[1, 0], [-1, 0], [0, 1], [0, -1]], _point


_add_door = (square, tiles, diggable) ->
  for step in _KING_MOVES
    neighbor = Point.sum square, step
    if _is_tile_blocked tiles[neighbor.x][neighbor.y]
      diggable[neigbor.x][neighbor.y] = false
  if (do Math.random) < 0.5
    tiles[square.x][square.y] = gen.Tile.DOOR

# Returns true if it is possible to erode the given square in the map.
# A square may be immune to erosion if:
#  - It has no free orthogonal neighbors. We want all rooms to be connected
#    by rook moves, even though the player can move diagonally.
#  - It is adjacent to squares in two different rooms. Eroding it would
#    connect those two rooms, which we don't want.
#
# If this method returns true, it will set room_index to be the index of
# the adjacent free square's room.
_can_erode_square = (rids, square, room_index) ->
  room_index[0] = 0
  has_free_orthogonal_neighbor = false
  min_unblocked_index = -1
  max_unblocked_index = -1
  gaps = 0
  for i in [0...8]
    step = _KING_MOVES[i]
    adjacent = rids[square.x + step.x][square.y + step.y]
    if adjacent == 0
      continue
    room_index[0] = adjacent
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

_get_debug_char_for_tile = (tile) ->
  result = ' .#+'[tile]
  if result? then result else '?'

_has_neighbor_in_room = (rids, square, room_index, neigbor_in_room) ->
  for step in _ROOK_MOVES
    neighbor_in_room[0] = Point.sum
    if rids[neighbor_in_room.x][neighbor_in_room.y] == room_index
      return true
  false

_in_bounds = (point, size) ->
  0 <= point.x < size.x and 0 <= point.y < size.y

_is_tile_blocked = (tile) ->
  tile == gen.Tile.DEFAULT or tile == gen.Tile.WALL
