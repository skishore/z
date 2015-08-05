@gen = @gen or {}


_2d_dropoff = (point, size) ->
  assert (size.x > 0 and size.y > 0)
  sample = new Point point.x/size.x, point.y/size.y
  (1 - Math.abs sample.x)*(1 - Math.abs sample.y)

_adjacent = (point1, point2) ->
  diff = point1.subtract point2
  (Math.abs diff.x) <= 1 and (Math.abs diff.y) <= 1

_get_offset = (size, point) ->
  result = new Point 0, 0
  if point.x == 0
    result.x = 1
  else if point.x == size.x - 1
    result.x = -1
  if point.y == 0
    result.y = 1
  else if point.y == size.y - 1
    result.y = -1
  result

_get_river = (size, start, end) ->
  dstart = _get_offset size, start
  dend = _get_offset size, end
  result = _get_river_inner size, (start.add dstart), (end.add dend)
  result.insert (start.add dstart)
  result.insert (end.add dend)
  if (dstart.dot dend) == 0
    _widen_river size, result, dstart
    _widen_river size, result, dend
  else
    direction = new Point 0, 0
    if (Math.abs dstart.x) + (Math.abs dend.x) > 0
      direction.y = if (start.add end).y > size.y - 1 then -1 else 1
    else if (Math.abs dstart.y) + (Math.abs dend.y) > 0
      direction.x = if (start.add end).x > size.x - 1 then -1 else 1
    _widen_river size, result, direction
  result

_get_river_inner = (size, start, end) ->
  if _adjacent start, end
    return new PointSet
  midpoint = _sample_midpoint size, start, end
  side1 = _get_river_inner size, start, midpoint
  side2 = _get_river_inner size, midpoint, end
  for point in do side2.keys
    side1.insert point
  side1.insert midpoint
  side1

_sample_midpoint = (size, start, end) ->
  center = radius = (size.subtract new Point 1, 1).scale 0.5
  rect_center = (start.add end).scale 0.5
  rect_radius = (start.subtract end).scale 0.5
  rect_radius.x = (Math.abs rect_radius.x) + 2
  rect_radius.y = (Math.abs rect_radius.y) + 2
  length = do (start.subtract end).length
  while true
    point = new Point (_.random 2, size.x - 3), (_.random 2, size.y - 3)
    continue if (do (point.subtract start).length) >= length
    continue if (do (point.subtract end).length) >= length
    centrality = _2d_dropoff (point.subtract center), radius
    inlineness = _2d_dropoff (point.subtract rect_center), rect_radius
    acceptance_probability = centrality*centrality*inlineness
    if (do Math.random) < acceptance_probability
      return point

_widen_river = (size, river, diff) ->
  for point in do river.keys
    found_neighbor = false
    for i in [1...Math.max size.x, size.y]
      if (river.contains point.add diff.scale i) or \
         (river.contains point.add diff.scale -i)
        found_neighbor = true
        break
    if not found_neighbor
      river.insert point.add diff


class gen.RiverMap
  constructor: (@size, @start, @end, verbose) ->
    while not @_try_build_map verbose
      true
 
  # Returns true if the map was successfully built.
  _try_build_map: (verbose) ->
    level = new gen.Level @size
    rects = []

    separation = 3
    rect = new gen.Rect (new Point @size.x - 2, @size.y - 2), (new Point 1, 1)
    level.place_rectangular_room rect, separation, rects

    river = _get_river @size, @start, @end
    for point in do river.keys
      level.tiles[point.x][point.y] = gen.Tile.DEFAULT

    do level.add_walls
    if verbose
      console.log "Final map:#{do level.to_debug_string}"
    true
