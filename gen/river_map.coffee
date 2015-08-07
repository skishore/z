@gen = @gen or {}


edge_penalty = (point, size) ->
  assert (size.x > 1 and size.y > 1)
  sample = new Point (2*point.x/(size.x - 1) - 1), (2*point.y/(size.y - 1) - 1)
  Math.max (Math.abs sample.x), (Math.abs sample.y)

in_bounds = (point, size) ->
  0 <= point.x < size.x and 0 <= point.y < size.y

point = (pair) -> new Point pair[0], pair[1]

ROOK_MOVES = _.map [[1, 0], [-1, 0], [0, 1], [0, -1]], point

_get_river = (size, start, end, options) ->
  options = options or {}
  centrality = (if options.centrality? then options.centrality else 0)
  randomness = (if options.randomness? then options.randomness else 0)
  windiness = (if options.windiness? then options.windiness else 0)
  length = (if options.length? then options.length else 0)
  correction = centrality/3 + randomness/2

  visited = new PointSet
  parents = new PointMap
  parents.set start, start
  distances = new PointMap
  distances.set start, 0

  while not visited.contains end
    best_distance = Infinity
    best_key = undefined
    for key in _.keys distances
      if distances[key] < best_distance
        best_distance = distances[key]
        best_key = key
    if not best_key?
      break
    delete distances[best_key]
    best_node = distances.extract best_key
    visited.insert best_node
    previous_step = best_node.subtract parents.get best_node
    for step in ROOK_MOVES
      child = best_node.add step
      if (not in_bounds child, size) or visited.contains child
        continue
      distance = best_distance \
                 + (centrality*edge_penalty child, size) \
                 + (randomness*do Math.random) \
                 + (windiness*previous_step.dot step) \
                 - length - correction
      distances.set child, distance
      parents.set child, best_node

  assert visited.contains end
  node = end
  path = [node]
  while not node.equals start
    node = parents.get node
    path.push node

  result = new PointSet
  for node in path
    result.insert node
  result


class gen.RiverMap
  constructor: (@size, @start, @end, @options, verbose) ->
    while not @_try_build_map verbose
      true
 
  # Returns true if the map was successfully built.
  _try_build_map: (verbose) ->
    level = new gen.Level @size
    rects = []

    separation = 3
    rect = new gen.Rect @size, (new Point 0, 0)
    level.place_rectangular_room rect, separation, rects

    @river = _get_river @size, @start, @end, @options
    for point in do @river.keys
      level.tiles[point.x][point.y] = gen.Tile.DEFAULT

    do level.add_walls
    if verbose
      console.log "Final map:#{do level.to_debug_string}"
    true
