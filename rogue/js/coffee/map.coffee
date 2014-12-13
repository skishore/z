get_random_tile = ->
  if (Util.randint 20) then '.' else 'X'


class @Map
  constructor: (@cols, @rows) ->
    @tiles = (('\0' for row in [0...@rows]) for col in [0...@cols])

  fill_randomly: ->
    for x in [0...@cols]
      for y in [0...@rows]
        @tiles[x][y] = do get_random_tile

  is_square_blocked: (x, y) ->
    if (0 <= x < @cols and 0 <= y < @rows)
      return @tiles[x][y] != '.'
    true
