class @View
  constructor: (@cols, @rows, player_position) ->
    @tiles = (('\0' for row in [0...@rows]) for col in [0...@cols])
    @player_position = new Point player_position.x, player_position.y
