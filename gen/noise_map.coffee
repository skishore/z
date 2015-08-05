@gen = @gen or {}


class gen.NoiseMap
  constructor: (@size, verbose) ->
    level = new gen.Level @size
    noise.seed do Math.random

    # scale is the wavelength of the Perlin noise.
    # threshold is the fraction of squares that are free.
    scale = 0.2
    threshold = 0.5
    free_squares = []

    for x in [0...@size.x]
      for y in [0...@size.y]
        sample = ((noise.perlin2 x/(scale*@size.x), y/(scale*@size.y)) + 1)/2
        level.tiles[x][y] = if sample > threshold \
                            then gen.Tile.DEFAULT else gen.Tile.FREE

    do level.add_walls
    @rooms = []
    @starting_square = new Point (Math.floor @size.x/2), (Math.floor @size.y/2)
    @tiles = level.tiles
    if verbose
      console.log "Final map:#{do level.to_debug_string}"
