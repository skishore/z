class @Point
  constructor: (@x, @y) ->

  equal: (other) ->
    @x == other.x and @y == other.y

  length: ->
    Math.sqrt @x*@x + @y*@y

  @sum: (point1, point2) ->
    new Point point1.x + point2.x, point1.y + point2.y


class @PointMap
  delete: (point) ->
    delete @["#{point.x},#{point.y}"]

  extract: (key) ->
    [x, y] = key.split ','
    new Point (parseInt x, 10), (parseInt y, 10)

  get: (point, fallback) ->
    result = @["#{point.x},#{point.y}"]
    if result? then result else fallback

  set: (point, value) ->
    @["#{point.x},#{point.y}"] = value


class @PointSet
  contains: (point) ->
    @["#{point.x},#{point.y}"]?

  delete: (point) ->
    delete @["#{point.x},#{point.y}"]

  insert: (point) ->
    @["#{point.x},#{point.y}"] = true
