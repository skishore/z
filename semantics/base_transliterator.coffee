@semantics = @semantics or {}

class semantics.BaseTransliterator
  constructor: (input) ->
    @error = undefined
    @output = ''
    @state = ''
    @state_indices = [0, 0]
    @_process input

  _process: (input) ->
    @_advance_state character, i for character, i in input
    if input.length > 0 and not @error?
      do @_pop_state

  _advance_state: (character, i) ->
    if @error?
      return
    if not @accept character
      if character == ' '
        do @_pop_state
        @output += ' '
      else
        @error = "Unexpected character at #{i}: #{character}"
        return
    next_state = @state + character
    if @is_valid_prefix next_state
      @state = next_state
      @state_indices[1] += 1
    else if @state == ''
      @error = "Unexpected character at #{i}: #{character}"
      return
    else
      do @_pop_state
      @_advance_state character, i

  _pop_state: ->
    assert @state.length > 0, 'Called pop_state without state!'
    do @pop_state
    @state = ''
    @state_indices = [@state_indices[1], @state_indices[1]]

  accept: (character) ->
    throw new Error 'accept is not implemented!'

  is_valid_prefix: (state) ->
    throw new Error 'is_valid_prefix is not implemented!'

  pop_state: ->
    throw new Error 'pop_state is not implemented!'

  @unsafe: (input) ->
    result = new @ input
    if result.error?
      throw new Error result.error
    result.output
