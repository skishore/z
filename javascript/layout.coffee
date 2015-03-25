class @BabelLayout
  constructor: (@scale, square, radius) ->
    @square = @scale*square
    @center = {x: @square*(radius - 1), y: @square*(radius - 1)}
    # This constant should match @text_arrow_size in triangle.less.
    text_arrow_size = 6
    hpadding = 0
    vpadding = 1
    @kClasses = ['bottom-right', 'bottom', 'bottom-left',
                 'right', 'left', 'top-right', 'top', 'top-left']
    @kOffsets = [
      [0.5*@square + text_arrow_size, -vpadding],
      [0.5*@square, -vpadding],
      [0.5*@square - text_arrow_size, -vpadding],
      [-hpadding, 0.5*@square],
      [@square + hpadding, 0.5*@square],
      [0.5*@square + text_arrow_size, @square + vpadding],
      [0.5*@square, @square + vpadding],
      [0.5*@square - text_arrow_size, @square + vpadding],
    ]

  place: (view, sprites) ->
    sprite_rects = []
    text_rects = []

    for id, sprite of sprites
      sprite_rects.push {
        x: @scale*sprite.x
        y: @scale*sprite.y
        w: @square
        h: @square
      }

    ids = []
    for id of view.sprites
      ids.push id
    ids = do ids.sort

    labels = []
    for sprite_view in (view.sprites[id] for id in ids)
      if sprite_view.label.length == 0
        continue
      if sprite_view.id not of sprites
        continue
      sprite = sprites[sprite_view.id]
      label = sprite_view.label
      direction = (@_best_direction sprite, label, sprite_rects, text_rects)
      text_rects.push (@_position sprite, direction, label)[0]
      labels.push @_label sprite, direction, label
    labels

  # Returns the area of the intersection of the two rectangles.
  _intersection: (r1, r2) ->
    (Math.max (Math.min r1.x + r1.w, r2.x + r2.w) - (Math.max r1.x, r2.x), 0) *
    (Math.max (Math.min r1.y + r1.h, r2.y + r2.h) - (Math.max r1.y, r2.y), 0)

  _score: (discriminant, offset, rect, sprite_rects, text_rects) ->
    score = 0
    if discriminant[0] == offset.x
      score += 2
    if discriminant[1] == offset.y
      score += 2
    if offset.y == 0
      score += 4
    for sprite_rect in sprite_rects
      score -= @_intersection rect, sprite_rect
    for text_rect in text_rects
      score -= @_intersection rect, text_rect
    score

  _sgn: (x) ->
    if x < -1 then -1 else if x > 1 then 1 else 0

  _best_direction: (sprite, label, sprite_rects, text_rects) ->
    discriminant = [@_sgn(@scale*sprite.x - @center.x),
                    @_sgn(@scale*sprite.y - @center.y)]
    best_score = -1 << 16
    best_index = -1
    for i in [0...8]
      [rect, offset] = @_position sprite, i, label
      score = @_score discriminant, offset, rect, sprite_rects, text_rects
      if score > best_score
        best_score = score
        best_index = i
    best_index

  _position: (sprite, direction, label) ->
    rect = {
      x: @scale*sprite.x
      y: @scale*sprite.y
      w: @square*label.length
      h: @square
    }
    offset = {x: 0, y: 0}
    if direction < 3
      rect.x += 0.5*@square
      rect.y -= @square
      offset.y = -1
    else if direction > 4
      rect.x += 0.5*@square
      rect.y += @square
      offset.y = 1
    if direction == 0 or direction == 3 or direction == 5
      rect.x -= rect.w
      offset.x = -1
    else if direction == 1 or direction == 6
      rect.x -= 0.5*rect.w
      offset.x = 0
    else
      offset.x = 1
    if direction == 4
      rect.x += @square
    [rect, offset]

  _label: (sprite, direction, label) ->
    assert 0 <= direction < 8
    offset = @kOffsets[direction]
    left: @scale*sprite.x + offset[0]
    top: @scale*sprite.y + offset[1]
    orientation: @kClasses[direction]
    text: label
