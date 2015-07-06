# Fast version of _.omit that takes a dict as the second parameter instead of
# a list and that does not support extra arguments.
_.fast_omit = (data, keys_to_drop) ->
  result = {}
  for key of data
    if key not of keys_to_drop
      result[key] = data[key]
  result

# Fast version of _.extend that does not support extra arguments.
_.fast_extend = (target, source) ->
  for key of source
    target[key] = source[key]
  target
