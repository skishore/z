if Meteor.isServer
  return

Template.grid.events {
  click: -> Session.set 'cells', iterate Session.get 'cells'
}

Template.grid.helpers {
  cells: ->
    result = transpose Session.get 'cells'
    for row, y in result
      for color, x in row
        row[x] = color: color
    result
}

transpose = (array) -> _.zip.apply _, array

sum_neighbors = (array, x, y) ->
  sum = 0
  for dx in [-1..1]
    for dy in [-1..1]
      sum += array[x + dx]?[y + dy] or 0
  sum - array[x][y]

live_probability = [
  [0.0, 0.1, 0.4, 0.8, 0.4, 0.2, 0.1, 0.1],
  [0.2, 0.4, 0.6, 0.8, 0.8, 0.6, 0.4, 0.2]
]

iterate = (array) ->
  result = _.map array, _.clone
  for x in [0...width]
    for y in [0...height]
      if x == 0 and (do Math.random) < 0.1
        result[x][y] = 1
        continue
      neighbors = (sum_neighbors array, x, y) + (Math.max 1 - x, 0)
      cutoff = live_probability[array[x][y]][neighbors]
      if (cutoff > 0 and (do Math.random) < cutoff) != (array[x][y] == 1)
        result[x][y] = if (do Math.random) < 0.2 \
                       then 1 - array[x][y] else array[x][y]
  result

width = 32
height = 16

cells = []
for i in [0...width]
  cells.push (0 for y in [0...height])

Session.set 'cells', cells
