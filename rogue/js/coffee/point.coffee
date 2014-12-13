class @Point
  constructor: (@x, @y) ->

  @sum: (point1, point2) ->
    new Point point1.x + point2.x, point1.y + point2.y
