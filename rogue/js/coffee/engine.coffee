OFFSETS =
  h: new Point -1, 0
  j: new Point 0, 1
  k: new Point 0, -1
  l: new Point 1, 0
  y: new Point -1, -1
  u: new Point 1, -1
  b: new Point -1, 1
  n: new Point 1, 1


class @Engine
  constructor: (cols, rows) ->
    @map = new Map cols,rows
    @player_position = new Point (Math.floor cols/2), (Math.floor rows/2)
    do @map.fill_randomly
    @map.tiles[@player_position.x][@player_position.y] = '.'

  do_command: (command) ->
    if command not of OFFSETS
      return false
    point = Point.sum @player_position, OFFSETS[command]
    if @map.is_square_blocked point.x, point.y
      return false
    @player_position = point
    true

  get_view: ->
    view = new View @map.cols, @map.rows, @player_position
    for x in [0...@map.cols]
      for y in [0...@map.rows]
        view.tiles[x][y] = @map.tiles[x][y]
    return view
