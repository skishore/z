@semantics = @semantics or {}

semantics.ENGLISH = {}
for char in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
  semantics.ENGLISH[char] = true
