class @Point
  constructor: (@x, @y) ->

  clone: ->
    new Point @x, @y

  equals: (other) ->
    @x == other.x and @y == other.y

  length: ->
    Math.sqrt @x*@x + @y*@y

  round: ->
    new Point (Math.round @x), (Math.round @y)

  scale: (factor) ->
    new Point factor*@x, factor*@y

  scale_to: (length) ->
    assert not do @zero
    @scale length/(do @length)

  zero: ->
    @x == 0 and @y == 0

  @difference: (point1, point2) ->
    new Point point1.x - point2.x, point1.y - point2.y

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
