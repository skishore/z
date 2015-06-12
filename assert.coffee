@assert = (condition, message) ->
  throw new Error message if not condition
