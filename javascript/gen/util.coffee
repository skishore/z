@gen = @gen or {}


@gen.construct_2d_array = (size, value) ->
  ((value for y in [0...size.y]) for x in [0...size.x])

@gen.rect_to_rect_distance = (rect1, rect2) ->
  distance = new Point(
      Math.max(rect1.position.x - rect2.position.x - rect2.size.x,
               rect2.position.x - rect1.position.x - rect1.size.x, 0),
      Math.max(rect1.position.y - rect2.position.y - rect2.size.y,
               rect2.position.y - rect1.position.y - rect1.size.y, 0))
  do distance.length


@Tile = {DEFAULT: 0, FREE: 1, WALL: 2, DOOR: 3, FENCE: 4}


class @gen.Rect
  constructor: (@size, @position) ->


class @gen.Room
  constructor: ->
    @squares = []

  get_random_square: ->
    _.sample @squares


class @gen.Level
  constructor: (@size) ->
    @tiles = gen.construct_2d_array @size, Tile.DEFAULT
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
              @tiles[square.x][square.y] == Tile.DEFAULT)
            @tiles[square.x][square.y] = Tile.WALL
 
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
        @tiles[node.x][node.y] = Tile::FREE
    _add_door truncated_path[1], @tiles, @diggable
    _add_door(truncated_path[truncated_path.length - 2], @tiles, @diggable)
    true

  # Runs an erosion step on the level. Each tile has a chance of being
  # converted to the types of the tiles around it.
  #
  # Islandness is between 0 and 16, with increasing islandness causing it to
  # be more likely to have random walls in the interior of a room.
  erode: (islandness) ->

  # Fills the rooms array with the final (non-rectangular) rooms.
  extract_final_rooms: (n, rooms) ->

  # Returns true and adds rect to rects if the room was successfully placed.
  place_rectangular_room: (rect, separation, rects) ->

  # Returns a human-readable serialization of the level.
  to_debug_string: (show_rooms) ->


_point = (pair) -> new Point pair[0], pair[1]

_BISHOP_MOVES = _.map [[1, 1], [1, -1], [-1, 1], [-1, -1]], _point

# IMPORTANT: The king moves are arranged in increasing order of angle.
_KING_MOVES = _.map [[1, 0], [1, 1], [0, 1], [-1, 1],
                     [-1, 0], [-1, -1], [0, -1], [1, -1]], _point

_ROOK_MOVES = _.map [[1, 0], [-1, 0], [0, 1], [0, -1]], _point

#console.log _ROOK_MOVES


_add_door = (square, tiles, diggable) ->
  for step in _KING_MOVES
    neighbor = Point.sum square, step
    if _is_tile_blocked tiles[neighbor.x][neighbor.y]
      diggable[neigbor.x][neighbor.y] = false
  if (do Math.random) < 0.5
    tiles[square.x][square.y] = Tile.DOOR

_has_neighbor_in_room = (rids, square, room_index, neigbor_in_room) ->
  for step in _ROOK_MOVES
    neighbor_in_room[0] = Point.sum
    if rids[neighbor_in_room.x][neighbor_in_room.y] == room_index
      return true
  false

_in_bounds = (point, size) ->
  0 <= point.x < size.x and 0 <= point.y < size.y

_is_tile_blocked = (tile) ->
  tile == Tile.DEFAULT or tile == Tile.WALL
