if Meteor.isServer
  return

Template.grid.events {
  click: -> Session.set 'cells', iterate Session.get 'cells'
}

Template.grid.helpers {
  cells: -> transpose Session.get 'cells'
}

transpose = (array) -> _.zip.apply _, array

sum_neighbors = (array, x, y) ->
  sum = 0
  for dx in [-1..1]
    for dy in [-1..1]
      sum += array[x + dx]?[y + dy] or 0
  sum - array[x][y]

iterate = (array) ->
  result = _.map array, _.clone
  for x in [0...width]
    for y in [0...height]
      neighbors = sum_neighbors array, x, y
      if array[x][y] == 1
        result[x][y] = if (neighbors == 2 or neighbors == 3) then 1 else 0
      else
        result[x][y] = if neighbors == 3 then 1 else 0
  result

width = 3
height = 3

cells = []
for i in [0...width]
  cells.push (0 for y in [0...height])

for x in [0...width]
  for y in [0...height]
    if (do Math.random) < 0.2
      cells[x][y] = 1

Session.set 'cells', cells
