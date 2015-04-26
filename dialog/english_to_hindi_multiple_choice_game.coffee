class @EnglishToHindiMultipleChoiceGame extends DialogPage
  @template = 'multiple_choice_translit'
  @height: '2.2em'
  @trap_input: false

  constructor: ->
    n = 3
    m = 4
    @permutation = _.shuffle [0...m]

    hindi = []
    english = []
    for i in [0...m]
      while true
        new_hindi = do semantics.Devanagari.get_segment
        new_english = semantics.HindiToEnglish.unsafe new_hindi
        if (english.indexOf new_english) < 0
          break
      hindi.push new_hindi
      english.push new_english

    @questions = (english[i] for i in [0...n])
    @answers = (hindi[j] for j in @permutation)
    @assignment = (undefined for i in [0...n])
    @classes = (undefined for i in [0...n])
    @_active = true

    # Fields needed to connect this game with the battle.
    @_num_enemies = m
    @_num_enemies_added = 0
    @_sid_to_index = {}

  add_enemy: (sid) ->
    @_sid_to_index[sid] = @_num_enemies_added
    @_num_enemies_added += 1

  get_num_enemies: ->
    return @_num_enemies

  get_label: (sid) ->
    if sid not of @_sid_to_index
      return undefined
    return {cls: 'hindi', text: @answers[@_sid_to_index[sid]]}

  get_data: ->
    data = {question: '', answer: ''}
    for question in @questions
      data.question += semantics.HindiToEnglish.english_to_display question
    for answer in @answers
      data.answer += answer
    data
