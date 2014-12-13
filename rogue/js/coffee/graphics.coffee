build_cache = (view) ->
  cache = ([] for x in [0...view.cols])
  for x in [0...view.cols]
    for y in [0...view.rows]
      text = view.tiles[x][y]
      cache[x].push
        class: 'light-gray'
        text: if text == '\0' then '#' else text
  cache[view.player_position.x][view.player_position.y] =
    class: 'light-gray-inv'
    text: '@'
  cache


class @Graphics
  constructor: (container, view) ->
    @target = $('<div>').addClass 'tty-map'
    @elements = ([] for col in [0...view.cols])
    for y in [0...view.rows]
      if y > 0
        @target.append $('<br>')
      for x in [0...view.cols]
        @elements[x].push $('<div>').addClass 'tile'
        @target.append @elements[x][y]
    @last_cache = (({} for y in [0...view.rows]) for x in [0...view.cols])
    @redraw view
    container.append @target

  redraw: (view) ->
    cache = build_cache view
    for x in [0...view.cols]
      for y in [0...view.rows]
        current = cache[x][y]
        last = @last_cache[x][y]
        if current.text != last.text
          @elements[x][y].text current.text
        if current.class != last.class
          (@elements[x][y].removeClass last.class).addClass current.class
    @last_cache = cache
