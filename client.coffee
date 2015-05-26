if Meteor.isServer
  return

Template.grid.helpers {
  cells: ->
    result = transpose Session.get 'cells'
    for row, y in result
      for color, x in row
        row[x] = color: color
    result
}

Cell = {OPEN: 0, GUARDED: 1, PLAYER: 2, ENEMY: 3}

transpose = (array) -> _.zip.apply _, array

keys = {h: [-1, 0], j: [0, 1], k: [0, -1], l: [1, 0], \
        y: [-1, -1], u: [1, -1], b: [-1, 1], n: [1, 1]}

count_neighbors = (array, x, y) ->
  sum = 0
  for dx in [-1..1]
    for dy in [-1..1]
      if (array[x + dx]?[y + dy] or 0) == Cell.ENEMY
        sum += 1
  sum

live_probability = [0.0, 0.1, 0.4, 0.8, 0.2, 0.1, 0.0, 0.0]

iterate = (array) ->
  result = _.map array, _.clone
  for x in [0...width]
    for y in [0...height]
      value = array[x][y]
      if value != Cell.OPEN and value != Cell.GUARDED
        result[x][y] = value
        continue
      neighbors = count_neighbors array, x, y
      cutoff = live_probability[neighbors]
      if cutoff > 0 and (do Math.random) < cutoff
        flip_probability = if value == Cell.OPEN then 0.1 else 0.02
        result[x][y] = if (do Math.random) < flip_probability \
                       then Cell.ENEMY else value
  result

width = 16
height = 8

cells = []
for i in [0...width]
  cells.push (0 for y in [0...height])

random_cell = -> [(_.random width - 1), (_.random height - 1)]

for i in [0...(_.random 2, 3)]
  [x, y] = do random_cell
  while cells[x][y] != 0
    [x, y] = do random_cell
  cells[x][y] = Cell.ENEMY

player = do random_cell
while cells[player[0]][player[1]] != 0
  player = do random_cell
cells[player[0]][player[1]] = Cell.PLAYER

window.onkeypress = (e) ->
  key = String.fromCharCode e.which
  if not (key of keys)
    return
  [x, y] = [player[0] + keys[key][0], player[1] + keys[key][1]]
  if not (0 <= x < width and 0 <= y < height)
    return
  cells = Session.get 'cells'
  if cells[x][y] != Cell.OPEN and cells[x][y] != Cell.GUARDED
    return
  cells[x][y] = Cell.PLAYER
  cells[player[0]][player[1]] = Cell.GUARDED
  [player[0], player[1]] = [x, y]
  Session.set 'cells', iterate cells

Session.set 'cells', cells
