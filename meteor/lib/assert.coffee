@assert = (condition, error) ->
  if not condition
    throw new Error error
