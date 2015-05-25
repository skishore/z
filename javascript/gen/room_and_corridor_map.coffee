@gen = @gen or {}


class gen.RoomAndCorridorMap
  constructor: (@size, verbose) ->
    while not @_try_build_map verbose
      true
 
  # Returns true if the map was successfully built.
  _try_build_map: (verbose) ->
    level = new gen.Level @size
    rects = []

    [min_size, max_size] = [6, 8]
    separation = 3
    tries = Math.floor @size.x*@size.y/(min_size*min_size)
    tries_left = tries

    while tries_left > 0
      size = new Point((_.random min_size, max_size),
                       (_.random min_size, max_size))
      rect = new gen.Rect(size, new Point((_.random 1, @size.x - size.x - 1),
                                          (_.random 1, @size.y - size.y - 1)))
      if not level.place_rectangular_room rect, separation, rects
        tries_left -= 1
    n = rects.length
    assert n > 0
    if verbose
      console.log "Placed #{n} rectangular rooms after #{tries} attempts."

    islandness = _.random 2
    for i in [0...3]
      level.erode islandness
    @rooms = level.extract_final_rooms n

    do level.add_walls
    @starting_square = do @rooms[0].get_random_square
    if verbose
      console.log "Final map:#{do level.to_debug_string}"
    true
