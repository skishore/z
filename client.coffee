if Meteor.isServer
  return

Template.grid.helpers {
  rows: -> Session.get 'grid'
}

CELL_SIZE = 20

chunk = (data) ->
  assert data.length > 0
  result = [{length: 0, value: data[0]}]
  for value in data
    if value != result[result.length - 1].value
      result.push {length: 0, value: value}
    result[result.length - 1].length += 1
  result

chunk_max_size = (data, value, max_size) ->
  result = []
  for block in data
    if block.value != value
      result.push block
      continue
    subblocks = []
    while block.length > max_size
      subblock = _.random 1, max_size
      subblocks.push subblock
      block.length -= subblock
    subblocks.push block.length
    for subblock in _.shuffle subblocks
      if (do Math.random) < 0.5
        result.push {length: subblock, value: block.value, \
                     text: get_word_for_length subblock}
      else
        result.push {length: subblock, value: 2}
  result

get_word_for_length = (length) ->
  word = ''
  while word.length == 0 or word.length > 3*length - 1
    word = _.sample WORDS
  word

generate_grid = (size) ->
  map = new gen.NoiseMap size, true
  grid = []
  for y in [0...size.y]
    row = (map.tiles[x][y] for x in [0...size.x])
    row = ((if tile == 2 then 0 else tile) for tile in row)
    row = chunk_max_size (chunk row), 0, 3
    for block in row
      border = if block.value == 1 then 0 else 2
      block.width = "#{CELL_SIZE*block.length - border}px"
      block.height = "#{CELL_SIZE - border}px"
    grid.push row
  Session.set 'grid', grid

keys = {h: [-1, 0], j: [0, 1], k: [0, -1], l: [1, 0], \
        y: [-1, -1], u: [1, -1], b: [-1, 1], n: [1, 1]}

window.onkeypress = (e) ->
  key = String.fromCharCode e.which

Meteor.startup ->
  generate_grid new Point 48, 24
