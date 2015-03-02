class @EnglishToHindi extends BaseTransliterator
  constructor: (input) ->
    @last_was_consonant = false
    super input

  accept: (character) ->
    ENGLISH[character]

  is_valid_prefix: (state) ->
    state == 'z' or state of TRANSLITERATIONS

  pop_state: ->
    hindi = TRANSLITERATIONS[@state]
    assert hindi?, "Unexpected state: #{@state}"
    is_consonant = hindi not of SIGNS
    if @last_was_consonant
      if is_consonant
        @output += VIRAMA
      else
        hindi = SIGNS[hindi]
    @output += hindi
    @last_was_consonant = is_consonant
