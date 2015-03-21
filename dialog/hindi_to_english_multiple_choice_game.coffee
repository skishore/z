class @HindiToEnglishMultipleChoiceGame extends DialogPage
  @template = 'multiple_choice_translit'
  @height: '3.6em'

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

    @questions = (hindi[i] for i in [0...n])
    @answers = (english[j] for j in @permutation)
    @assignment = (undefined for i in [0...n])
    @classes = (undefined for i in [0...n])
    @_active = true

  accepts_input: (char) ->
    semantics.ENGLISH[char] or char == '\b' or char == '\r'

  active: -> @_active

  get_data: ->
    data = {questions: [], answers: []}
    for question, i in @questions
      data.questions.push
        class: @classes[i]
        left: "#{(Math.floor 100*(2*i + 1)/(2*@questions.length))}%"
        text: question
    labels = 'ASDF'
    for answer, i in @answers
      j = @assignment.indexOf i
      p = if j < 0 then i else j
      n = if j < 0 then @answers.length else @questions.length
      data.answers.push
        class: @classes[j]
        left: "#{(Math.floor 100*(2*p + 1)/(2*n))}%"
        label: labels[i]
        text: semantics.HindiToEnglish.english_to_display answer
    data

  on_input: (char) ->
    if char == '\b'
      return do @_on_backspace
    else if char == '\r'
      return do @_on_enter
    @_on_typed_char char

  _on_backspace: ->
    index = -1
    for i in [0...@questions.length]
      if @assignment[i]? and @classes[i] != 'correct'
        index = i
    if index < 0
      return false
    @assignment[index] = undefined
    @classes[index] = undefined
    true

  _on_enter: ->
    mistake = false
    correct = false
    for i in [0...@questions.length]
      if @assignment[i]? and @classes[i] != 'correct'
        if @permutation[@assignment[i]] == i
          @classes[i] = 'correct'
          correct = true
        else
          @assignment[i] = undefined
          @classes[i] = 'wrong'
          mistake = true
    @_active = not _.all (cls == 'correct' for cls in @classes)
    correct or mistake

  _on_typed_char: (char) ->
    answer = {a: 0, s: 1, d: 2, f: 3}[char]
    if not answer?
      return false
    assignment = @assignment.indexOf answer
    if assignment >= 0
      return false
    index = -1
    for i in (do [0...@questions.length].reverse)
      if not @assignment[i]? and @classes[i] != 'correct'
        index = i
    if index < 0
      return false
    @assignment[index] = answer
    @classes[index] = 'done'
    true
