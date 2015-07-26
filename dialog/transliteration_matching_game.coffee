class @TransliterationMatchingGame extends DialogPage
  @template = 'short_answer_translit'
  @height: '3.6em'
  @trap_input: false

  constructor: ->
    EWWT = semantics.ENGLISH_WORDS_WITH_TRANSLITERATIONS
    RT = semantics.REVERSE_TRANSLITERATIONS
    n = _.random 2, 4
    data = undefined
    while (not data?) or not @_distinct (row[0] for row in data)
      data = (_.sample EWWT for i in [0...n])
    @questions = []
    @answers = []
    for row in data
      characters = (_.sample RT[entry] for entry in row[1])
      @questions.push semantics.Devanagari.concatenate characters
      @answers.push row[0]
    @flipped = (do Math.random) < 0.5
    if @flipped
      [@questions, @answers] = [@answers, @questions]
    # Fields needed to connect this game with the battle.
    @enemies_attacked = []
    @sid_to_answer = {}

  get_data: ->
    data = {class: "matching-game #{if @flipped then 'flipped'}", segments: []}
    for question, i in @questions
      data.segments.push
        segment: question
        entry: {text: if i < @enemies_attacked.length then @answers[i]}
        class: if i < @enemies_attacked.length then 'correct'
        width: "#{Math.floor 100/@answers.length}%"
    data

  # Combat-dialog-specific handlers follow. TODO(skishore): These handlers
  # should be moved to a generic combat-handling base class.

  add_enemy: (sid) ->
    index = (_.keys @sid_to_answer).length
    @sid_to_answer[sid] = @answers[index]

  can_attack: (sid) ->
    @sid_to_answer[sid] == @answers[@enemies_attacked.length]

  get_label: (sid) ->
    if sid not of @sid_to_answer
      return undefined
    class: if @flipped then 'hindi' else 'english'
    text: @sid_to_answer[sid]

  get_num_enemies: ->
    return @answers.length

  on_attack: (sid) ->
    if sid not of @sid_to_answer or not @can_attack sid
      return DialogAttackResult.WRONG_ENEMY
    @enemies_attacked.push sid
    DialogManager._redraw 'current'
    if @enemies_attacked.length == @answers.length
      return DialogAttackResult.COMBAT_WON
    return DialogAttackResult.RIGHT_ENEMY

  _distinct: (values) ->
    (_.union values).length == values.length
