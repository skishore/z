if Meteor.isServer
  return

Template.grid.helpers {
  rows: -> Session.get 'grid'
}

CELL_SIZE = 16

chunk = (data) ->
  assert data.length > 0
  result = [{length: 0, value: 0}]
  for value in data
    if value != result[result.length - 1].value
      result.push {length: 0, value: value}
    result[result.length - 1].length += 1
  result

generate_grid = (size) ->
  map = new gen.RoomAndCorridorMap size
  grid = []
  for y in [0...size.y]
    row = chunk (map.tiles[x][y] for x in [0...size.x])
    for block in row
      block.width = "#{CELL_SIZE*block.length}px"
    grid.push row
  Session.set 'grid', grid

keys = {h: [-1, 0], j: [0, 1], k: [0, -1], l: [1, 0], \
        y: [-1, -1], u: [1, -1], b: [-1, 1], n: [1, 1]}

window.onkeypress = (e) ->
  key = String.fromCharCode e.which

Meteor.startup ->
  generate_grid new Point 48, 24
