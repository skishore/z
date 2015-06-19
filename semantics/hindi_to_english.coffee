@semantics = @semantics or {}

REPLACEMENTS = {
  A: 'ā'
  I: 'ī'
  U: 'ū'
  D: 'ḍ'
  T: 'ṭ'
}

SIGNS = semantics.Devanagari.SIGNS
REVERSE_SIGNS = semantics.Devanagari.REVERSE_SIGNS
VIRAMA = semantics.Devanagari.VIRAMA


class semantics.HindiToEnglish extends semantics.BaseTransliterator
  constructor: (input) ->
    @last_was_consonant = false
    super input

  accept: (character) ->
    character of semantics.TRANSLITERATIONS or
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
        @output += semantics.TRANSLITERATIONS[REVERSE_SIGNS[@state]]
      @last_was_consonant = false
      return
    english = semantics.TRANSLITERATIONS[@state]
    assert english?, "Unexpected state: #{@state}"
    if @last_was_consonant
      @output += semantics.TRANSLITERATIONS[REVERSE_SIGNS['']]
    @output += english
    @last_was_consonant = @state not of SIGNS

  @english_to_display: (english) ->
    for character, replacement of REPLACEMENTS
      english = english.replace character, replacement
    english
