class @EnglishToHindiMultipleChoiceGame extends DialogPage
  @template = 'multiple_choice_translit'
  @height: '1.6em'
  @trap_input: false

  constructor: ->
    # TODO(skishore): There is a race condition in the game engine where,
    # if there are two distinct enemies with the same answer on them,
    # they can be attacked and destroyed simultaneously. This condition exists
    # because the API between the game engine and the dialog is now extremely
    # loose and complicated. As a temporary workaround, we ignore all
    # transliteration problems which contain duplicated answers.
    data = undefined
    while (not data?) or not @_distinct data[1]
      data = _.sample semantics.ENGLISH_WORDS_WITH_TRANSLITERATIONS
    [@english, @hindi] = data
    RT = semantics.REVERSE_TRANSLITERATIONS
    @answers = (_.sample RT[hindi] for hindi in @hindi)
    # Fields needed to connect this game with the battle.
    @enemies_attacked = []
    @sid_to_answer = {}

  get_data: ->
    characters = (@sid_to_answer[sid] for sid in @enemies_attacked)
    question: @english
    answer: semantics.Devanagari.concatenate characters

  # Combat-dialog-specific handlers follow. TODO(skishore): These handlers
  # should be moved to a generic combat-handling base class.

  add_enemy: (sid) ->
    index = (_.keys @sid_to_answer).length
    @sid_to_answer[sid] = @answers[index]

  can_attack: (sid) ->
    transliteration = semantics.TRANSLITERATIONS[@sid_to_answer[sid]]
    transliteration == @hindi[@enemies_attacked.length]

  get_label: (sid) ->
    if sid not of @sid_to_answer
      return undefined
    class: 'hindi character'
    text: @sid_to_answer[sid]

  get_num_enemies: ->
    return @answers.length

  on_attack: (sid) ->
    if sid not of @sid_to_answer or not @can_attack sid
      return DialogAttackResult.WRONG_ENEMY
    @enemies_attacked.push sid
    DialogManager._redraw 'current'
    if @enemies_attacked.length == @hindi.length
      return DialogAttackResult.COMBAT_WON
    return DialogAttackResult.RIGHT_ENEMY

  _distinct: (values) ->
    (_.union values).length == values.length
