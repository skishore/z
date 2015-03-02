Math.randint = (a, b) ->
  if not b?
    [a, b] = [0, a]
  a + Math.floor (b - a)*Math.random()

Math.randelt = (list) ->
  list[Math.randint list.length]
