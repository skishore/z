class @EnglishToHindiMultipleChoiceGame extends DialogPage
  @template = 'multiple_choice_translit'
  @height: '1.6em'
  @trap_input: false

  constructor: ->
    n = _.random 4, 6
    m = n
    @permutation = _.shuffle [0...m]

    hindi = []
    english = []
    for i in [0...m]
      while true
        new_hindi = _.sample semantics.Devanagari.ALPHABET
        new_english = semantics.HindiToEnglish.unsafe new_hindi
        if (english.indexOf new_english) < 0
          break
      hindi.push new_hindi
      english.push new_english

    @questions = (english[i] for i in [0...n])
    @answers = (hindi[j] for j in @permutation)
    @_active = true

    # Fields needed to connect this game with the battle.
    @_num_enemies = m
    @_num_enemies_added = 0
    @_num_enemies_hit = 0
    @_sid_to_index = {}

  add_enemy: (sid) ->
    @_sid_to_index[sid] = @_num_enemies_added
    @_num_enemies_added += 1

  get_num_enemies: ->
    return @_num_enemies

  can_attack: (sid) ->
    if sid not of @_sid_to_index
      return false
    index = @_sid_to_index[sid]
    @permutation[index] == @_num_enemies_hit

  on_attack: (sid) ->
    if sid not of @_sid_to_index
      return DialogAttackResult.WRONG_ENEMY
    index = @_sid_to_index[sid]
    if @permutation[index] != @_num_enemies_hit
      return DialogAttackResult.WRONG_ENEMY
    @_num_enemies_hit += 1
    DialogManager._redraw 'current'
    if @_num_enemies_hit == @questions.length
      return DialogAttackResult.COMBAT_WON
    return DialogAttackResult.RIGHT_ENEMY

  get_data: ->
    data = {question: '', answer: ''}
    for question in @questions
      data.question += semantics.HindiToEnglish.english_to_display question
    for i in [0...@_num_enemies_hit]
      data.answer += @answers[@permutation.indexOf i]
    data

  get_label: (sid) ->
    if sid not of @_sid_to_index
      return undefined
    return {cls: 'hindi', text: @answers[@_sid_to_index[sid]]}
