class @HindiToEnglish extends BaseTransliterator
  constructor: (input) ->
    @last_was_consonant = false
    super input

  accept: (character) ->
    character of REVERSE_TRANSLITERATIONS or
    character of REVERSE_SIGNS or
    character == VIRAMA

  is_valid_prefix: (state) ->
    state.length == 1

  pop_state: ->
    if @state of REVERSE_SIGNS or @state == VIRAMA
      if not @last_was_consonant
        @error = "Unexpected conjuct at #{@state_indices[0]}: #{@state}"
        return
      if @state of REVERSE_SIGNS
        @output += REVERSE_TRANSLITERATIONS[REVERSE_SIGNS[@state]]
      @last_was_consonant = false
      return
    english = REVERSE_TRANSLITERATIONS[@state]
    assert english?, "Unexpected state: #{@state}"
    if @last_was_consonant
      @output += REVERSE_TRANSLITERATIONS[REVERSE_SIGNS['']]
    @output += english
    @last_was_consonant = @state not of SIGNS
